"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     842                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Moral Philosophy Debates - Chat-Style Main Application

Chat-ähnliche GUI für moralphilosophische Debatten über Weltnachrichten.
Die Philosophen (Kant, Mill, Rawls, etc.) debattieren als Chat-Teilnehmer.
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from typing import Optional, List
from datetime import datetime

from models import (
    NewsArticle, DebateSession, PhilosophySchool, ChatMessage,
    ArgumentDimension, PHILOSOPHY_PROFILES, MessageType
)
from news_researcher import NewsResearcher
from debate_chat import DebateChatManager
from moral_engine import SimpleLLMBackend
from themis_client import MoralDebateClient


# Configuration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 1400
WINDOW_HEIGHT = 900


class PhilosophyChatApp:
    """
    Chat-Style-Anwendung für Moralphilosophische Debatten.
    
    Philosophen (Kant, Mill, Rawls, etc.) debattieren über News in einem Chat-Format.
    """
    
    def __init__(self, root: tk.Tk):
        """
        Initialize the application.
        
        Args:
            root: Tkinter root widget
        """
        self.root = root
        self.root.title("ThemisDB - Philosophen-Chat: Ethik-Debatten")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # Initialize components
        self.client = MoralDebateClient(host=THEMIS_HOST, port=THEMIS_PORT)
        self.news_researcher = NewsResearcher()
        self.llm_backend = SimpleLLMBackend()
        self.chat_manager = DebateChatManager(llm_backend=self.llm_backend)
        
        # State
        self.current_articles: List[NewsArticle] = []
        self.selected_article: Optional[NewsArticle] = None
        self.current_debate: Optional[DebateSession] = None
        
        # Chat colors for philosophers
        self.philosopher_colors = {
            # AI Synthesizer
            "KI (AI Synthesizer)": "#ff1493",  # Deep Pink for AI
            # Practical Philosophy
            "Immanuel Kant": "#3498db",
            "John Stuart Mill": "#27ae60",
            "W.D. Ross": "#9b59b6",
            "John Rawls": "#e67e22",
            "Aristoteles": "#e74c3c",
            "Carol Gilligan": "#f39c12",
            "Jürgen Habermas": "#1abc9c",
            # Theoretical Philosophy
            "René Descartes": "#8e44ad",
            "David Hume": "#16a085",
            "Edmund Husserl": "#c0392b",
            "William James": "#d35400",
            "Ludwig Wittgenstein": "#2c3e50",
            "Jean-Paul Sartre": "#34495e",
            # Lebensphilosophie
            "Friedrich Nietzsche": "#8b0000",  # Dark red
            "Arthur Schopenhauer": "#2f4f4f",  # Dark slate gray
            "Wilhelm Dilthey": "#556b2f",  # Dark olive green
            # Political Philosophy
            "Karl Marx": "#dc143c",  # Crimson red
            "Hannah Arendt": "#9370db",  # Medium purple
            # Ancient Greek Philosophy
            "Sokrates": "#e8b44d",
            "Protagoras": "#95a5a6",
            # Meta-Ethics
            "G.E. Moore": "#6c5ce7",
            "A.J. Ayer": "#fd79a8",
            "J.L. Mackie": "#636e72",
            "Simon Blackburn": "#00b894",
            "R.M. Hare": "#fdcb6e",
            # Historical Schools
            "Seneca": "#b8860b",
            "Epikur": "#dda15e",
            "Thomas von Aquin": "#8b4513",
            "Konfuzius": "#dc143c",
            "Buddha": "#ff6347",
            "Philippa Foot": "#20b2aa"
        }
        
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
        
        # Main container: News selection (left) + Chat (right)
        main_container = tk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_container.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left panel: News selection
        self._create_news_panel(main_container)
        
        # Right panel: Chat interface
        self._create_chat_panel(main_container)
        
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
            text="💬 Philosophen-Chat: Ethik-Debatten",
            bg="#2c3e50",
            fg="white",
            font=("Arial", 18, "bold")
        )
        title_label.pack(side=tk.LEFT, padx=20, pady=15)
        
        # Subtitle
        subtitle_label = tk.Label(
            header_frame,
            text="Kant, Mill, Rawls & Co. debattieren über Weltnachrichten",
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
    
    def _create_news_panel(self, parent):
        """Creates the news selection panel."""
        news_panel = tk.Frame(parent, bg="white", width=400)
        parent.add(news_panel)
        
        # Header
        header = tk.Frame(news_panel, bg="#34495e", height=50)
        header.pack(fill=tk.X)
        header.pack_propagate(False)
        
        tk.Label(
            header,
            text="📰 Nachrichtenauswahl",
            bg="#34495e",
            fg="white",
            font=("Arial", 12, "bold")
        ).pack(side=tk.LEFT, padx=15, pady=10)
        
        # Controls
        control_frame = tk.Frame(news_panel, bg="white", pady=10)
        control_frame.pack(fill=tk.X, padx=10)
        
        tk.Label(
            control_frame,
            text="Kategorie:",
            bg="white",
            font=("Arial", 9)
        ).pack(side=tk.LEFT, padx=(0, 5))
        
        self.category_combo = ttk.Combobox(
            control_frame,
            values=["Alle", "Technology", "Politics", "Health", "Environment", "Society"],
            state="readonly",
            width=12,
            font=("Arial", 9)
        )
        self.category_combo.set("Alle")
        self.category_combo.pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            control_frame,
            text="🔄",
            command=self._load_news,
            bg="#3498db",
            fg="white",
            font=("Arial", 9),
            width=3
        ).pack(side=tk.LEFT, padx=5)
        
        # News list
        list_frame = tk.Frame(news_panel, bg="white")
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        scrollbar = tk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.news_listbox = tk.Listbox(
            list_frame,
            yscrollcommand=scrollbar.set,
            font=("Arial", 9),
            selectmode=tk.SINGLE,
            bg="#f8f9fa"
        )
        self.news_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.news_listbox.yview)
        
        self.news_listbox.bind("<<ListboxSelect>>", self._on_news_select)
        
        # Article preview
        preview_frame = tk.LabelFrame(
            news_panel,
            text="Vorschau",
            font=("Arial", 9, "bold"),
            bg="white",
            padx=5,
            pady=5
        )
        preview_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        self.preview_text = scrolledtext.ScrolledText(
            preview_frame,
            height=6,
            font=("Arial", 8),
            wrap=tk.WORD,
            bg="#f8f9fa",
            state=tk.DISABLED
        )
        self.preview_text.pack(fill=tk.BOTH, expand=True)
        
        # Start debate button
        tk.Button(
            news_panel,
            text="▶️ Debatte starten",
            command=self._start_debate,
            bg="#27ae60",
            fg="white",
            font=("Arial", 11, "bold"),
            height=2
        ).pack(fill=tk.X, padx=10, pady=(0, 10))
    
    def _create_chat_panel(self, parent):
        """Creates the chat interface panel."""
        chat_panel = tk.Frame(parent, bg="white")
        parent.add(chat_panel)
        
        # Chat header
        chat_header = tk.Frame(chat_panel, bg="#34495e", height=50)
        chat_header.pack(fill=tk.X)
        chat_header.pack_propagate(False)
        
        self.chat_title_label = tk.Label(
            chat_header,
            text="💬 Wählen Sie eine Nachricht für die Debatte",
            bg="#34495e",
            fg="white",
            font=("Arial", 12, "bold")
        )
        self.chat_title_label.pack(side=tk.LEFT, padx=15, pady=10)
        
        # Dimension selector
        dimension_frame = tk.Frame(chat_panel, bg="#ecf0f1", height=50)
        dimension_frame.pack(fill=tk.X)
        dimension_frame.pack_propagate(False)
        
        tk.Label(
            dimension_frame,
            text="Dimensionen:",
            bg="#ecf0f1",
            font=("Arial", 9, "bold")
        ).pack(side=tk.LEFT, padx=10)
        
        self.dimension_vars = {}
        dimensions = [
            # Practical Philosophy
            ("Moralisch", ArgumentDimension.MORAL),
            ("Sozial", ArgumentDimension.SOCIAL),
            ("Politisch", ArgumentDimension.POLITICAL),
            # Theoretical Philosophy
            ("Erkenntnistheoretisch", ArgumentDimension.EPISTEMOLOGICAL),
            ("Metaphysisch", ArgumentDimension.METAPHYSICAL),
            # Meta-Ethics
            ("Metaethisch", ArgumentDimension.METAETHICAL),
            ("Normativ", ArgumentDimension.NORMATIVE),
        ]
        
        for label, dim in dimensions:
            var = tk.BooleanVar(value=True if dim == ArgumentDimension.MORAL else False)
            self.dimension_vars[dim] = var
            cb = tk.Checkbutton(
                dimension_frame,
                text=label,
                variable=var,
                bg="#ecf0f1",
                font=("Arial", 8)
            )
            cb.pack(side=tk.LEFT, padx=3)
        
        # Chat display area
        chat_display_frame = tk.Frame(chat_panel, bg="white")
        chat_display_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Scrollbar
        scrollbar = tk.Scrollbar(chat_display_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Chat text widget
        self.chat_text = tk.Text(
            chat_display_frame,
            yscrollcommand=scrollbar.set,
            font=("Arial", 10),
            wrap=tk.WORD,
            bg="#f8f9fa",
            state=tk.DISABLED,
            padx=10,
            pady=10
        )
        self.chat_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.chat_text.yview)
        
        # Configure tags for different philosophers
        for name, color in self.philosopher_colors.items():
            self.chat_text.tag_config(f"name_{name}", foreground=color, font=("Arial", 10, "bold"))
        self.chat_text.tag_config("timestamp", foreground="#7f8c8d", font=("Arial", 8))
        self.chat_text.tag_config("dimension", foreground="#9b59b6", font=("Arial", 8, "italic"))
        self.chat_text.tag_config("response_to", foreground="#3498db", font=("Arial", 9, "italic"))
        
        # Control buttons
        button_frame = tk.Frame(chat_panel, bg="white", height=60)
        button_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        button_frame.pack_propagate(False)
        
        tk.Button(
            button_frame,
            text="⏩ Nächste Runde",
            command=self._advance_debate,
            bg="#3498db",
            fg="white",
            font=("Arial", 10, "bold"),
            width=15
        ).pack(side=tk.LEFT, padx=5, pady=10)
        
        tk.Button(
            button_frame,
            text="💾 Speichern",
            command=self._save_debate,
            bg="#95a5a6",
            fg="white",
            font=("Arial", 10),
            width=12
        ).pack(side=tk.LEFT, padx=5, pady=10)
        
        self.round_label = tk.Label(
            button_frame,
            text="Runde: -",
            bg="white",
            font=("Arial", 10, "bold")
        )
        self.round_label.pack(side=tk.RIGHT, padx=10, pady=10)
        
        # Time display label
        self.time_label = tk.Label(
            button_frame,
            text="⏱️ Zeit: --:--",
            bg="white",
            font=("Arial", 10)
        )
        self.time_label.pack(side=tk.RIGHT, padx=10, pady=10)
    
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
                display_text = f"• {article.title[:60]}..."
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
            self._display_article_preview(self.selected_article)
    
    def _display_article_preview(self, article: NewsArticle):
        """Displays article preview."""
        self.preview_text.config(state=tk.NORMAL)
        self.preview_text.delete(1.0, tk.END)
        
        text = f"{article.title}\n\n"
        text += f"{article.content[:300]}...\n"
        
        self.preview_text.insert(1.0, text)
        self.preview_text.config(state=tk.DISABLED)
    
    def _start_debate(self):
        """Starts a new debate on the selected article."""
        if not self.selected_article:
            messagebox.showwarning(
                "Keine Auswahl",
                "Bitte wählen Sie zuerst einen Nachrichtenartikel aus."
            )
            return
        
        # Get selected dimensions
        selected_dims = [
            dim for dim, var in self.dimension_vars.items() if var.get()
        ]
        
        if not selected_dims:
            messagebox.showwarning(
                "Keine Dimensionen",
                "Bitte wählen Sie mindestens eine Dimension aus."
            )
            return
        
        self._set_status("Starte Debatte...", "info")
        
        try:
            # Create debate session
            from moral_engine import MoralDebateEngine
            debate_engine = MoralDebateEngine(self.llm_backend)
            session = debate_engine.create_debate_session(self.selected_article)
            
            # Start chat debate
            # Select philosophers based on dimensions
            has_practical = any(dim in [ArgumentDimension.MORAL, ArgumentDimension.SOCIAL, 
                                       ArgumentDimension.POLITICAL, ArgumentDimension.ETHICAL]
                              for dim in selected_dims)
            has_theoretical = any(dim in [ArgumentDimension.EPISTEMOLOGICAL, ArgumentDimension.METAPHYSICAL,
                                         ArgumentDimension.LOGICAL, ArgumentDimension.ONTOLOGICAL]
                                for dim in selected_dims)
            has_metaethical = any(dim in [ArgumentDimension.METAETHICAL, ArgumentDimension.NORMATIVE]
                                 for dim in selected_dims)
            
            philosophies = []
            if has_practical:
                philosophies.extend([
                    PhilosophySchool.KANT,
                    PhilosophySchool.UTILITARIANISM,
                    PhilosophySchool.VIRTUE_ETHICS,
                    PhilosophySchool.SOCRATIC,
                    PhilosophySchool.STOICISM
                ])
            if has_theoretical:
                philosophies.extend([
                    PhilosophySchool.RATIONALISM,
                    PhilosophySchool.ARISTOTELIAN,
                    PhilosophySchool.EMPIRICISM
                ])
            if has_metaethical:
                philosophies.extend([
                    PhilosophySchool.MORAL_REALISM,
                    PhilosophySchool.INTUITIONISM,
                    PhilosophySchool.PRESCRIPTIVISM,
                    PhilosophySchool.NATURALISM
                ])
            
            # If multiple categories, limit to avoid too many participants
            if sum([has_practical, has_theoretical, has_metaethical]) >= 2:
                philosophies = [
                    PhilosophySchool.SOCRATIC,
                    PhilosophySchool.KANT,
                    PhilosophySchool.UTILITARIANISM,
                    PhilosophySchool.STOICISM,
                    PhilosophySchool.MORAL_REALISM
                ]
            
            session = self.chat_manager.start_debate_chat(
                session,
                selected_dims,
                philosophies
            )
            
            self.current_debate = session
            
            # Update UI
            self.chat_title_label.config(
                text=f"💬 Debatte: {session.debate_topic[:60]}..."
            )
            self.round_label.config(text=f"Runde: {session.current_round}")
            self._update_time_display()
            
            # Display chat
            self._display_chat()
            
            # Store in ThemisDB
            self.client.store_debate_session(session)
            
            # Start timer update
            self._start_timer_update()
            
            self._set_status("Debatte erfolgreich gestartet", "success")
        
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", f"Konnte Debatte nicht starten:\n{str(e)}")
    
    def _advance_debate(self):
        """Advances the debate to the next round."""
        if not self.current_debate:
            messagebox.showwarning(
                "Keine Debatte",
                "Bitte starten Sie zuerst eine Debatte."
            )
            return
        
        # Check time limit before advancing
        if self.current_debate.is_time_limit_exceeded():
            elapsed = self.current_debate.get_elapsed_time_minutes()
            messagebox.showwarning(
                "Zeitlimit überschritten",
                f"Die Debatte hat das Zeitlimit von {self.current_debate.max_duration_minutes} Minuten überschritten.\n"
                f"Verstrichene Zeit: {elapsed:.1f} Minuten.\n\n"
                f"Die Debatte wird beendet."
            )
            self.current_debate.completed_at = datetime.now()
            self.client.store_debate_session(self.current_debate)
            return
        
        if self.current_debate.current_round >= 4:
            messagebox.showinfo(
                "Debatte abgeschlossen",
                "Die Debatte hat alle Runden durchlaufen."
            )
            self.current_debate.completed_at = datetime.now()
            self.client.store_debate_session(self.current_debate)
            return
        
        # Warn if approaching time limit (< 10 minutes remaining)
        remaining = self.current_debate.get_remaining_time_minutes()
        if remaining < 10 and remaining > 0:
            response = messagebox.askyesno(
                "Zeitwarnung",
                f"Nur noch {remaining:.1f} Minuten verbleiben.\n"
                f"Möchten Sie fortfahren?"
            )
            if not response:
                return
        
        self._set_status("Generiere nächste Runde...", "info")
        
        try:
            self.current_debate = self.chat_manager.advance_debate_round(
                self.current_debate
            )
            
            self.round_label.config(text=f"Runde: {self.current_debate.current_round}")
            self._update_time_display()
            
            # Display updated chat
            self._display_chat()
            
            # Store updated session
            self.client.store_debate_session(self.current_debate)
            
            # Check if completed due to time limit
            if self.current_debate.completed_at:
                self._set_status("Debatte wegen Zeitlimit beendet", "warning")
                messagebox.showinfo(
                    "Debatte beendet",
                    f"Die Debatte wurde nach {self.current_debate.get_elapsed_time_minutes():.1f} Minuten beendet."
                )
            else:
                self._set_status(f"Runde {self.current_debate.current_round} generiert", "success")
        
        except ValueError as e:
            # Time limit exceeded
            self._set_status("Zeitlimit überschritten", "warning")
            messagebox.showwarning("Zeitlimit", str(e))
            self.current_debate.completed_at = datetime.now()
            self.client.store_debate_session(self.current_debate)
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", f"Konnte Runde nicht generieren:\n{str(e)}")
    
    def _display_chat(self):
        """Displays the chat messages."""
        if not self.current_debate:
            return
        
        self.chat_text.config(state=tk.NORMAL)
        self.chat_text.delete(1.0, tk.END)
        
        # Group messages by dimension
        by_dimension = {}
        for msg in self.current_debate.chat_messages:
            if msg.dimension not in by_dimension:
                by_dimension[msg.dimension] = []
            by_dimension[msg.dimension].append(msg)
        
        # Display messages grouped by dimension
        for dimension, messages in by_dimension.items():
            dimension_names = {
                # Practical Philosophy
                ArgumentDimension.MORAL: "🔴 MORALISCHE DIMENSION",
                ArgumentDimension.SOCIAL: "🔵 SOZIALE DIMENSION",
                ArgumentDimension.POLITICAL: "🟢 POLITISCHE DIMENSION",
                ArgumentDimension.ETHICAL: "🟡 ETHISCHE DIMENSION",
                ArgumentDimension.ECONOMIC: "💰 ÖKONOMISCHE DIMENSION",
                ArgumentDimension.LEGAL: "⚖️ RECHTLICHE DIMENSION",
                # Theoretical Philosophy
                ArgumentDimension.EPISTEMOLOGICAL: "🧠 ERKENNTNISTHEORETISCHE DIMENSION",
                ArgumentDimension.METAPHYSICAL: "🌌 METAPHYSISCHE DIMENSION",
                ArgumentDimension.LOGICAL: "🔷 LOGISCHE DIMENSION",
                ArgumentDimension.ONTOLOGICAL: "💠 ONTOLOGISCHE DIMENSION",
                ArgumentDimension.PHENOMENOLOGICAL: "👁️ PHÄNOMENOLOGISCHE DIMENSION",
                # Meta-Ethics
                ArgumentDimension.METAETHICAL: "🔬 METAETHISCHE DIMENSION",
                ArgumentDimension.NORMATIVE: "📏 NORMATIVE DIMENSION",
                ArgumentDimension.APPLIED: "🎯 ANGEWANDTE ETHIK",
                ArgumentDimension.DESCRIPTIVE: "📊 DESKRIPTIVE ETHIK"
            }
            
            self.chat_text.insert(tk.END, "\n")
            self.chat_text.insert(tk.END, f"{dimension_names.get(dimension, dimension.value.upper())}\n", "dimension")
            self.chat_text.insert(tk.END, "=" * 80 + "\n\n")
            
            for msg in messages:
                self._display_message(msg)
        
        self.chat_text.config(state=tk.DISABLED)
        self.chat_text.see(tk.END)
    
    def _display_message(self, message: ChatMessage):
        """Displays a single chat message."""
        profile = PHILOSOPHY_PROFILES[message.philosophy_school]
        philosopher_name = profile.philosopher_name
        
        # Message type indicator
        type_indicators = {
            MessageType.STATEMENT: "💬",
            MessageType.COUNTER: "↩️",
            MessageType.REBUTTAL: "🔄",
            MessageType.SYNTHESIS: "🤝"
        }
        indicator = type_indicators.get(message.message_type, "💬")
        
        # If this is a response, show who is being responded to
        response_info = ""
        if message.responds_to:
            # Find the original message
            original = next(
                (msg for msg in self.current_debate.chat_messages if msg.id == message.responds_to),
                None
            )
            if original:
                original_profile = PHILOSOPHY_PROFILES[original.philosophy_school]
                response_info = f" → @{original_profile.philosopher_name}"
        
        # Name and timestamp
        time_str = message.timestamp.strftime("%H:%M")
        self.chat_text.insert(tk.END, f"{indicator} ")
        self.chat_text.insert(tk.END, f"{philosopher_name}", f"name_{philosopher_name}")
        if response_info:
            self.chat_text.insert(tk.END, response_info, "response_to")
        self.chat_text.insert(tk.END, f"  {time_str}\n", "timestamp")
        
        # Message content
        self.chat_text.insert(tk.END, f"{message.content}\n\n")
    
    def _save_debate(self):
        """Saves the current debate."""
        if not self.current_debate:
            messagebox.showwarning("Keine Debatte", "Keine aktive Debatte zum Speichern.")
            return
        
        try:
            success = self.client.store_debate_session(self.current_debate)
            if success:
                self._set_status("Debatte erfolgreich gespeichert", "success")
                messagebox.showinfo("Erfolg", "Debatte wurde in ThemisDB gespeichert.")
            else:
                self._set_status("Fehler beim Speichern", "error")
                messagebox.showwarning("Warnung", "Debatte konnte nicht gespeichert werden.")
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", str(e))
    
    def _update_time_display(self):
        """Updates the time display for the current debate."""
        if not self.current_debate:
            self.time_label.config(text="⏱️ Zeit: --:--", fg="black")
            return
        
        elapsed = self.current_debate.get_elapsed_time_minutes()
        remaining = self.current_debate.get_remaining_time_minutes()
        
        # Format elapsed time
        elapsed_min = int(elapsed)
        elapsed_sec = int((elapsed - elapsed_min) * 60)
        
        # Format remaining time
        remaining_min = int(abs(remaining))
        remaining_sec = int((abs(remaining) - remaining_min) * 60)
        
        # Color coding based on remaining time
        if remaining < 0:
            # Time exceeded
            time_text = f"⏱️ Zeit: {elapsed_min:02d}:{elapsed_sec:02d} (Überschritten!)"
            color = "#e74c3c"  # Red
        elif remaining < 10:
            # Less than 10 minutes remaining - warning
            time_text = f"⏱️ Zeit: {elapsed_min:02d}:{elapsed_sec:02d} | Verbleibend: {remaining_min:02d}:{remaining_sec:02d}"
            color = "#f39c12"  # Orange
        else:
            # Normal
            time_text = f"⏱️ Zeit: {elapsed_min:02d}:{elapsed_sec:02d} / {self.current_debate.max_duration_minutes} Min"
            color = "#27ae60"  # Green
        
        self.time_label.config(text=time_text, fg=color)
    
    def _start_timer_update(self):
        """Starts periodic timer updates."""
        def update():
            if self.current_debate and not self.current_debate.completed_at:
                self._update_time_display()
                # Schedule next update in 1 second
                self.root.after(1000, update)
        
        # Start the update loop
        update()
    
    def run(self):
        """Runs the application."""
        self.root.mainloop()


def main():
    """Main function."""
    root = tk.Tk()
    app = PhilosophyChatApp(root)
    app.run()


if __name__ == "__main__":
    main()
