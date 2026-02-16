#!/usr/bin/env python3
"""
Standalone Moral Philosophy Dialectic - GUI Frontend
Chat-ähnliche tkinter GUI für moralphilosophische Debatten

Features:
- Chat-style interface with color-coded philosopher messages
- URL input for newspaper articles as debate topics
- User can contribute moral considerations
- Philosophers respond to user messages
- Persistent storage via SQLite
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
from datetime import datetime
import threading
import uuid
from typing import Optional, List

# Import from standalone implementation
from standalone_moral_dialectic import (
    PhilosophySchool, ArgumentDimension, MessageType,
    ChatMessage, DebateSession,
    SQLiteDebateStore, OllamaBackend, MoralDialecticEngine,
    PHILOSOPHY_PROFILES
)


# Color scheme for different philosophers
PHILOSOPHER_COLORS = {
    PhilosophySchool.KANT: "#4A90E2",  # Blue
    PhilosophySchool.UTILITARIANISM: "#50C878",  # Green
    PhilosophySchool.VIRTUE_ETHICS: "#9B59B6",  # Purple
    PhilosophySchool.SOCRATIC: "#E67E22",  # Orange
    PhilosophySchool.STOICISM: "#95A5A6",  # Gray
    PhilosophySchool.USER: "#E74C3C"  # Red
}


class MoralDialecticGUI:
    """
    Chat-Style GUI for Moral Philosophy Debates.
    
    Users can input topics/URLs, add their own moral considerations,
    and the philosophical perspectives respond interactively.
    """
    
    def __init__(self, root: tk.Tk):
        """Initialize the GUI.
        
        Args:
            root: Tkinter root window
        """
        self.root = root
        self.root.title("Standalone Moral Dialectic - Philosophy Chat")
        self.root.geometry("1200x800")
        
        # Initialize backend components
        self.db_store = SQLiteDebateStore("moral_debates_gui.db")
        self.llm_backend = OllamaBackend()
        self.engine = MoralDialecticEngine(self.llm_backend, self.db_store)
        
        # Current session
        self.current_session: Optional[DebateSession] = None
        self.debate_active = False
        
        # Check Ollama availability
        self.ollama_available = self.llm_backend.check_availability()
        
        # Build GUI
        self._build_gui()
        
        # Show welcome message
        self._show_welcome_message()
        
    def _build_gui(self):
        """Build the GUI layout."""
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(2, weight=1)
        
        # ===== Top Section: Topic/URL Input =====
        topic_frame = ttk.LabelFrame(main_frame, text="Debate Topic", padding="10")
        topic_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        topic_frame.columnconfigure(1, weight=1)
        
        ttk.Label(topic_frame, text="URL/Topic:").grid(row=0, column=0, sticky=tk.W)
        self.topic_entry = ttk.Entry(topic_frame, width=50)
        self.topic_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(5, 5))
        self.topic_entry.insert(0, "Künstliche Intelligenz in der Medizin")
        
        ttk.Label(topic_frame, text="Ethical Question:").grid(row=1, column=0, sticky=tk.W, pady=(5, 0))
        self.question_entry = ttk.Entry(topic_frame, width=50)
        self.question_entry.grid(row=1, column=1, sticky=(tk.W, tk.E), padx=(5, 5), pady=(5, 0))
        self.question_entry.insert(0, "Sollte eine KI über Leben und Tod entscheiden dürfen?")
        
        self.start_button = ttk.Button(topic_frame, text="Start Debate", command=self._start_debate)
        self.start_button.grid(row=0, column=2, rowspan=2, padx=(5, 0))
        
        # ===== Philosophy Selection =====
        phil_frame = ttk.LabelFrame(main_frame, text="Select Philosophers", padding="10")
        phil_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        self.philosophy_vars = {}
        col = 0
        for phil in [PhilosophySchool.KANT, PhilosophySchool.UTILITARIANISM, 
                     PhilosophySchool.VIRTUE_ETHICS, PhilosophySchool.SOCRATIC, 
                     PhilosophySchool.STOICISM]:
            var = tk.BooleanVar(value=True)
            self.philosophy_vars[phil] = var
            cb = ttk.Checkbutton(phil_frame, text=PHILOSOPHY_PROFILES[phil]["name"], variable=var)
            cb.grid(row=0, column=col, padx=10)
            col += 1
        
        # ===== Chat Display Area =====
        chat_frame = ttk.LabelFrame(main_frame, text="Debate Chat", padding="10")
        chat_frame.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 10))
        chat_frame.columnconfigure(0, weight=1)
        chat_frame.rowconfigure(0, weight=1)
        
        # Chat text widget with scrollbar
        self.chat_display = scrolledtext.ScrolledText(
            chat_frame,
            wrap=tk.WORD,
            width=100,
            height=30,
            font=("Arial", 10)
        )
        self.chat_display.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.chat_display.config(state=tk.DISABLED)
        
        # Configure text tags for colored messages
        for phil, color in PHILOSOPHER_COLORS.items():
            self.chat_display.tag_config(phil.value, foreground=color, font=("Arial", 10, "bold"))
        
        # ===== User Input Area =====
        input_frame = ttk.LabelFrame(main_frame, text="Your Moral Consideration", padding="10")
        input_frame.grid(row=3, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        input_frame.columnconfigure(0, weight=1)
        
        self.user_input = tk.Text(input_frame, wrap=tk.WORD, width=80, height=3, font=("Arial", 10))
        self.user_input.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=(0, 5))
        
        self.send_button = ttk.Button(input_frame, text="Send Message", command=self._send_user_message)
        self.send_button.grid(row=0, column=1)
        self.send_button.config(state=tk.DISABLED)
        
        # Bind Enter key (Ctrl+Enter to send)
        self.user_input.bind('<Control-Return>', lambda e: self._send_user_message())
        
        # ===== Status Bar =====
        status_frame = ttk.Frame(main_frame)
        status_frame.grid(row=4, column=0, sticky=(tk.W, tk.E))
        
        self.status_label = ttk.Label(status_frame, text="Ready", relief=tk.SUNKEN)
        self.status_label.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        if not self.ollama_available:
            self.status_label.config(text="⚠️  Ollama not available - Please start Ollama server", foreground="red")
    
    def _show_welcome_message(self):
        """Display welcome message in chat."""
        welcome = """
╔══════════════════════════════════════════════════════════════════╗
║     Welcome to Standalone Moral Philosophy Dialectic Chat       ║
╚══════════════════════════════════════════════════════════════════╝

This is a chat-like interface for philosophical debates on ethical questions.

How to use:
1. Enter a topic or URL for a news article in the "Topic" field
2. Enter an ethical question to debate
3. Select which philosophers should participate
4. Click "Start Debate" to begin
5. Add your own moral considerations in the input field
6. The philosophers will respond to your messages!

Available Philosophers:
- Immanuel Kant (Categorical Imperative)
- John Stuart Mill (Utilitarianism)
- Aristotle (Virtue Ethics)
- Socrates (Dialectical Method)
- Epictetus (Stoicism)

Press Ctrl+Enter to send your message.
"""
        self._add_system_message(welcome)
    
    def _add_system_message(self, text: str):
        """Add a system message to the chat display.
        
        Args:
            text: Message text
        """
        self.chat_display.config(state=tk.NORMAL)
        self.chat_display.insert(tk.END, text + "\n")
        self.chat_display.see(tk.END)
        self.chat_display.config(state=tk.DISABLED)
    
    def _add_chat_message(self, message: ChatMessage):
        """Add a chat message to the display.
        
        Args:
            message: ChatMessage to display
        """
        profile = PHILOSOPHY_PROFILES[message.philosophy_school]
        timestamp = message.timestamp.strftime("%H:%M:%S")
        
        self.chat_display.config(state=tk.NORMAL)
        
        # Add separator for readability
        self.chat_display.insert(tk.END, "\n" + "─" * 80 + "\n")
        
        # Add philosopher name with color
        self.chat_display.insert(tk.END, f"[{timestamp}] {profile['name']}", message.philosophy_school.value)
        
        # Add dimension/type info
        if message.message_type == MessageType.STATEMENT:
            self.chat_display.insert(tk.END, f" ({message.dimension.value} perspective)")
        elif message.message_type == MessageType.COUNTER:
            self.chat_display.insert(tk.END, " (counter-argument)")
        elif message.message_type == MessageType.USER:
            self.chat_display.insert(tk.END, " (moral consideration)")
        
        self.chat_display.insert(tk.END, ":\n")
        
        # Add message content
        self.chat_display.insert(tk.END, f"{message.content}\n")
        
        self.chat_display.see(tk.END)
        self.chat_display.config(state=tk.DISABLED)
    
    def _start_debate(self):
        """Start a new philosophical debate."""
        if not self.ollama_available:
            messagebox.showerror(
                "Ollama Not Available",
                "Ollama is not running. Please start Ollama:\n\n"
                "1. Run: ollama serve\n"
                "2. Ensure model is downloaded: ollama pull llama3.2"
            )
            return
        
        topic = self.topic_entry.get().strip()
        question = self.question_entry.get().strip()
        
        if not topic or not question:
            messagebox.showwarning("Input Required", "Please enter both topic and ethical question.")
            return
        
        # Get selected philosophers
        selected_philosophers = [
            phil for phil, var in self.philosophy_vars.items() if var.get()
        ]
        
        if not selected_philosophers:
            messagebox.showwarning("Selection Required", "Please select at least one philosopher.")
            return
        
        # Disable start button during debate
        self.start_button.config(state=tk.DISABLED)
        self.send_button.config(state=tk.DISABLED)
        self.status_label.config(text="Starting debate...")
        
        # Run debate in separate thread to keep GUI responsive
        thread = threading.Thread(target=self._run_debate, args=(topic, question, selected_philosophers))
        thread.daemon = True
        thread.start()
    
    def _run_debate(self, topic: str, question: str, philosophers: List[PhilosophySchool]):
        """Run the debate in a background thread.
        
        Args:
            topic: Debate topic
            question: Ethical question
            philosophers: List of participating philosophers
        """
        try:
            # Create new session
            self.current_session = DebateSession(
                id=f"debate_{uuid.uuid4().hex[:12]}",
                topic=topic,
                ethical_question=question
            )
            
            self._add_system_message(f"\n{'='*80}")
            self._add_system_message(f"NEW DEBATE STARTED")
            self._add_system_message(f"Topic: {topic}")
            self._add_system_message(f"Question: {question}")
            self._add_system_message(f"{'='*80}\n")
            
            # Use only moral and ethical dimensions for GUI
            dimensions = [ArgumentDimension.MORAL, ArgumentDimension.ETHICAL]
            
            # Round 1: Initial statements
            self._update_status("Round 1: Generating initial statements...")
            self._add_system_message("\n🔹 Round 1: Initial Philosophical Statements\n")
            
            for philosophy in philosophers:
                for dimension in dimensions:
                    self._update_status(f"Generating {PHILOSOPHY_PROFILES[philosophy]['name']} - {dimension.value}...")
                    message = self.engine._generate_statement(philosophy, dimension, question, self.current_session)
                    self.current_session.messages.append(message)
                    self.root.after(0, self._add_chat_message, message)
            
            # Save after round 1
            self.db_store.save_debate(self.current_session)
            
            # Round 2: Counter-arguments
            self._update_status("Round 2: Generating counter-arguments...")
            self._add_system_message("\n🔹 Round 2: Counter-Arguments\n")
            
            statements = [msg for msg in self.current_session.messages if msg.message_type == MessageType.STATEMENT]
            
            for philosophy in philosophers:
                opposing_statements = [s for s in statements if s.philosophy_school != philosophy]
                if opposing_statements:
                    import random
                    target = random.choice(opposing_statements)
                    self._update_status(f"Generating counter from {PHILOSOPHY_PROFILES[philosophy]['name']}...")
                    counter = self.engine._generate_counter(philosophy, target, question, self.current_session)
                    self.current_session.messages.append(counter)
                    self.root.after(0, self._add_chat_message, counter)
            
            # Mark as completed
            self.current_session.completed_at = datetime.now()
            self.db_store.save_debate(self.current_session)
            
            # Enable user interaction
            self.debate_active = True
            self.root.after(0, lambda: self.send_button.config(state=tk.NORMAL))
            
            self._add_system_message(f"\n{'='*80}")
            self._add_system_message("Debate completed! You can now add your moral considerations.")
            self._add_system_message(f"Debate ID: {self.current_session.id}")
            self._add_system_message(f"{'='*80}\n")
            
            self._update_status(f"Debate ready - {len(self.current_session.messages)} messages")
            self.root.after(0, lambda: self.start_button.config(state=tk.NORMAL))
            
        except Exception as e:
            error_msg = f"Error during debate: {str(e)}"
            self._add_system_message(f"\n❌ {error_msg}\n")
            self._update_status("Error occurred")
            self.root.after(0, lambda: messagebox.showerror("Error", error_msg))
            self.root.after(0, lambda: self.start_button.config(state=tk.NORMAL))
    
    def _send_user_message(self):
        """Send user's moral consideration."""
        if not self.debate_active:
            messagebox.showinfo("No Active Debate", "Please start a debate first.")
            return
        
        user_text = self.user_input.get("1.0", tk.END).strip()
        if not user_text:
            return
        
        # Create user message
        user_message = ChatMessage(
            id=f"msg_{uuid.uuid4().hex[:16]}",
            philosophy_school=PhilosophySchool.USER,
            message_type=MessageType.USER,
            dimension=ArgumentDimension.MORAL,  # Default dimension for user
            content=user_text
        )
        
        # Add to session and display
        self.current_session.messages.append(user_message)
        self._add_chat_message(user_message)
        
        # Clear input
        self.user_input.delete("1.0", tk.END)
        
        # Save session
        self.db_store.save_debate(self.current_session)
        
        # Disable input while generating responses
        self.send_button.config(state=tk.DISABLED)
        self.user_input.config(state=tk.DISABLED)
        
        # Generate responses from philosophers in background thread
        thread = threading.Thread(target=self._generate_responses_to_user, args=(user_message,))
        thread.daemon = True
        thread.start()
    
    def _generate_responses_to_user(self, user_message: ChatMessage):
        """Generate philosophical responses to user's message.
        
        Args:
            user_message: The user's message to respond to
        """
        try:
            # Get selected philosophers
            selected_philosophers = [
                phil for phil, var in self.philosophy_vars.items() if var.get()
            ]
            
            self._add_system_message("\n🔹 Philosophers responding to your consideration...\n")
            
            # Generate response from each philosopher
            for philosophy in selected_philosophers:
                self._update_status(f"Generating response from {PHILOSOPHY_PROFILES[philosophy]['name']}...")
                
                profile = PHILOSOPHY_PROFILES[philosophy]
                
                # Create prompt for response to user
                prompt = f"""The user contributed this moral consideration:
"{user_message.content}"

Respond to the user's point from your philosophical perspective. 
Engage with their argument respectfully and provide your philosophical analysis.
Keep your response under 150 words."""
                
                content = self.llm_backend.generate(
                    prompt=prompt,
                    system_prompt=profile["system_prompt"]
                )
                
                # Create response message
                response = ChatMessage(
                    id=f"msg_{uuid.uuid4().hex[:16]}",
                    philosophy_school=philosophy,
                    message_type=MessageType.COUNTER,  # Response to user
                    dimension=ArgumentDimension.MORAL,
                    content=content,
                    responds_to=user_message.id
                )
                
                # Add to session and display
                self.current_session.messages.append(response)
                self.root.after(0, self._add_chat_message, response)
            
            # Save session
            self.db_store.save_debate(self.current_session)
            
            # Re-enable input
            self.root.after(0, lambda: self.send_button.config(state=tk.NORMAL))
            self.root.after(0, lambda: self.user_input.config(state=tk.NORMAL))
            
            self._add_system_message("\n💬 Responses complete. You can continue the discussion!\n")
            self._update_status(f"Ready - {len(self.current_session.messages)} messages")
            
        except Exception as e:
            error_msg = f"Error generating responses: {str(e)}"
            self._add_system_message(f"\n❌ {error_msg}\n")
            self._update_status("Error occurred")
            self.root.after(0, lambda: messagebox.showerror("Error", error_msg))
            self.root.after(0, lambda: self.send_button.config(state=tk.NORMAL))
            self.root.after(0, lambda: self.user_input.config(state=tk.NORMAL))
    
    def _update_status(self, text: str):
        """Update status bar.
        
        Args:
            text: Status text
        """
        self.root.after(0, lambda: self.status_label.config(text=text))
    
    def run(self):
        """Start the GUI event loop."""
        self.root.mainloop()
    
    def __del__(self):
        """Cleanup on exit."""
        if hasattr(self, 'db_store'):
            self.db_store.close()


def main():
    """Main entry point for the GUI application."""
    root = tk.Tk()
    app = MoralDialecticGUI(root)
    app.run()


if __name__ == "__main__":
    main()
