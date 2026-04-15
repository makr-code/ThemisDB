"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     492                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Coding Platform - Main Application

A desktop application for managing code snippets with ThemisDB,
featuring web scraping, semantic search, and VSCode integration.
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
from typing import Optional
import os

from themis_client import MockThemisDBClient, ThemisDBClient
from models import CodeSnippet, ScrapingJobConfig, SourceType
from web_scraper import ScraperManager
from code_indexer import CodeIndexer


class CodingPlatformApp:
    """Main application window."""
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("ThemisDB Coding Platform")
        self.root.geometry("1200x800")
        
        # Initialize ThemisDB client (using mock for demo)
        self.use_mock = not os.getenv("USE_REAL_THEMISDB", "").lower() == "true"
        if self.use_mock:
            self.client = MockThemisDBClient()
            self.status_text = "Using Mock Client (for demo)"
        else:
            self.client = ThemisDBClient()
            if self.client.health_check():
                self.status_text = "Connected to ThemisDB"
            else:
                self.status_text = "Failed to connect to ThemisDB"
                messagebox.showwarning("Connection", "Could not connect to ThemisDB server")
        
        # Initialize managers
        self.scraper_manager = ScraperManager(self.client)
        self.code_indexer = CodeIndexer(self.client)
        
        # Setup UI
        self.setup_ui()
        
        # Load initial data
        self.refresh_snippets()
    
    def setup_ui(self):
        """Setup the user interface."""
        # Menu bar
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="New Snippet", command=self.new_snippet)
        file_menu.add_command(label="Import...", command=self.import_file)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self.show_about)
        
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)
        
        # Tab control
        self.notebook = ttk.Notebook(main_frame)
        self.notebook.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Tabs
        self.create_snippets_tab()
        self.create_search_tab()
        self.create_scraper_tab()
        self.create_stats_tab()
        
        # Status bar
        status_frame = ttk.Frame(main_frame)
        status_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=(5, 0))
        
        self.status_label = ttk.Label(status_frame, text=self.status_text)
        self.status_label.pack(side=tk.LEFT)
    
    def create_snippets_tab(self):
        """Create the snippets management tab."""
        snippets_frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(snippets_frame, text="Snippets")
        
        # Configure grid
        snippets_frame.columnconfigure(0, weight=1)
        snippets_frame.columnconfigure(1, weight=2)
        snippets_frame.rowconfigure(1, weight=1)
        
        # Left panel - Snippet list
        left_frame = ttk.LabelFrame(snippets_frame, text="Snippets", padding="5")
        left_frame.grid(row=0, column=0, rowspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(0, 5))
        
        # Filter frame
        filter_frame = ttk.Frame(left_frame)
        filter_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(filter_frame, text="Language:").pack(side=tk.LEFT)
        self.language_filter = ttk.Combobox(filter_frame, width=15, state="readonly")
        self.language_filter.pack(side=tk.LEFT, padx=5)
        self.language_filter['values'] = ('All', 'python', 'javascript', 'java', 'cpp', 'rust', 'go')
        self.language_filter.current(0)
        self.language_filter.bind('<<ComboboxSelected>>', lambda e: self.refresh_snippets())
        
        # Snippet listbox
        self.snippets_listbox = tk.Listbox(left_frame, height=20)
        self.snippets_listbox.pack(fill=tk.BOTH, expand=True)
        self.snippets_listbox.bind('<<ListboxSelect>>', self.on_snippet_select)
        
        # Buttons
        button_frame = ttk.Frame(left_frame)
        button_frame.pack(fill=tk.X, pady=(5, 0))
        
        ttk.Button(button_frame, text="New", command=self.new_snippet).pack(side=tk.LEFT, padx=2)
        ttk.Button(button_frame, text="Delete", command=self.delete_snippet).pack(side=tk.LEFT, padx=2)
        ttk.Button(button_frame, text="Refresh", command=self.refresh_snippets).pack(side=tk.LEFT, padx=2)
        
        # Right panel - Snippet details
        right_frame = ttk.LabelFrame(snippets_frame, text="Snippet Details", padding="5")
        right_frame.grid(row=0, column=1, rowspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Title
        ttk.Label(right_frame, text="Title:").pack(anchor=tk.W)
        self.title_entry = ttk.Entry(right_frame)
        self.title_entry.pack(fill=tk.X, pady=(0, 5))
        
        # Description
        ttk.Label(right_frame, text="Description:").pack(anchor=tk.W)
        self.description_entry = ttk.Entry(right_frame)
        self.description_entry.pack(fill=tk.X, pady=(0, 5))
        
        # Language and Framework
        info_frame = ttk.Frame(right_frame)
        info_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(info_frame, text="Language:").pack(side=tk.LEFT)
        self.language_entry = ttk.Entry(info_frame, width=15)
        self.language_entry.pack(side=tk.LEFT, padx=(5, 10))
        
        ttk.Label(info_frame, text="Framework:").pack(side=tk.LEFT)
        self.framework_entry = ttk.Entry(info_frame, width=15)
        self.framework_entry.pack(side=tk.LEFT, padx=5)
        
        # Tags
        ttk.Label(right_frame, text="Tags (comma-separated):").pack(anchor=tk.W)
        self.tags_entry = ttk.Entry(right_frame)
        self.tags_entry.pack(fill=tk.X, pady=(0, 5))
        
        # Code
        ttk.Label(right_frame, text="Code:").pack(anchor=tk.W)
        self.code_text = scrolledtext.ScrolledText(right_frame, height=20, width=60)
        self.code_text.pack(fill=tk.BOTH, expand=True, pady=(0, 5))
        
        # Save button
        ttk.Button(right_frame, text="Save Snippet", command=self.save_snippet).pack()
    
    def create_search_tab(self):
        """Create the search tab."""
        search_frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(search_frame, text="Search")
        
        # Search input
        search_input_frame = ttk.LabelFrame(search_frame, text="Semantic Search", padding="5")
        search_input_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(search_input_frame, text="Query:").pack(side=tk.LEFT)
        self.search_entry = ttk.Entry(search_input_frame, width=50)
        self.search_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        self.search_entry.bind('<Return>', lambda e: self.perform_search())
        
        ttk.Button(search_input_frame, text="Search", command=self.perform_search).pack(side=tk.LEFT)
        
        # Results
        results_frame = ttk.LabelFrame(search_frame, text="Results", padding="5")
        results_frame.pack(fill=tk.BOTH, expand=True)
        
        self.search_results = scrolledtext.ScrolledText(results_frame, height=25)
        self.search_results.pack(fill=tk.BOTH, expand=True)
    
    def create_scraper_tab(self):
        """Create the web scraper tab."""
        scraper_frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(scraper_frame, text="Web Scraper")
        
        # Scraper input
        input_frame = ttk.LabelFrame(scraper_frame, text="Scraping Configuration", padding="5")
        input_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(input_frame, text="Source Type:").grid(row=0, column=0, sticky=tk.W)
        self.scraper_type = ttk.Combobox(input_frame, width=20, state="readonly")
        self.scraper_type['values'] = ('GitHub Repository', 'Stack Overflow', 'Documentation')
        self.scraper_type.current(0)
        self.scraper_type.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=5, pady=2)
        
        ttk.Label(input_frame, text="URL:").grid(row=1, column=0, sticky=tk.W)
        self.scraper_url = ttk.Entry(input_frame, width=50)
        self.scraper_url.grid(row=1, column=1, sticky=(tk.W, tk.E), padx=5, pady=2)
        
        ttk.Label(input_frame, text="Max Files:").grid(row=2, column=0, sticky=tk.W)
        self.max_files = ttk.Entry(input_frame, width=10)
        self.max_files.insert(0, "100")
        self.max_files.grid(row=2, column=1, sticky=tk.W, padx=5, pady=2)
        
        ttk.Button(input_frame, text="Start Scraping", command=self.start_scraping).grid(row=3, column=0, columnspan=2, pady=10)
        
        input_frame.columnconfigure(1, weight=1)
        
        # Scraper log
        log_frame = ttk.LabelFrame(scraper_frame, text="Scraping Log", padding="5")
        log_frame.pack(fill=tk.BOTH, expand=True)
        
        self.scraper_log = scrolledtext.ScrolledText(log_frame, height=20)
        self.scraper_log.pack(fill=tk.BOTH, expand=True)
    
    def create_stats_tab(self):
        """Create the statistics tab."""
        stats_frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(stats_frame, text="Statistics")
        
        self.stats_text = scrolledtext.ScrolledText(stats_frame, height=30)
        self.stats_text.pack(fill=tk.BOTH, expand=True)
        
        ttk.Button(stats_frame, text="Refresh Statistics", command=self.refresh_statistics).pack(pady=5)
        
        self.refresh_statistics()
    
    # Event handlers
    
    def refresh_snippets(self):
        """Refresh the snippets list."""
        self.snippets_listbox.delete(0, tk.END)
        
        language = self.language_filter.get()
        if language == 'All':
            language = None
        
        snippets = self.client.list_snippets(language=language, limit=100)
        
        for snippet in snippets:
            self.snippets_listbox.insert(tk.END, f"{snippet.title} ({snippet.language})")
            self.snippets_listbox.snippet_data = snippets  # Store for later retrieval
    
    def on_snippet_select(self, event):
        """Handle snippet selection."""
        selection = self.snippets_listbox.curselection()
        if not selection:
            return
        
        index = selection[0]
        snippets = getattr(self.snippets_listbox, 'snippet_data', [])
        if index >= len(snippets):
            return
        
        snippet = snippets[index]
        
        # Populate form
        self.title_entry.delete(0, tk.END)
        self.title_entry.insert(0, snippet.title)
        
        self.description_entry.delete(0, tk.END)
        self.description_entry.insert(0, snippet.description)
        
        self.language_entry.delete(0, tk.END)
        self.language_entry.insert(0, snippet.language)
        
        self.framework_entry.delete(0, tk.END)
        if snippet.framework:
            self.framework_entry.insert(0, snippet.framework)
        
        self.tags_entry.delete(0, tk.END)
        self.tags_entry.insert(0, ', '.join(snippet.tags))
        
        self.code_text.delete(1.0, tk.END)
        self.code_text.insert(1.0, snippet.code)
    
    def new_snippet(self):
        """Clear form for new snippet."""
        self.title_entry.delete(0, tk.END)
        self.description_entry.delete(0, tk.END)
        self.language_entry.delete(0, tk.END)
        self.framework_entry.delete(0, tk.END)
        self.tags_entry.delete(0, tk.END)
        self.code_text.delete(1.0, tk.END)
        
        # Focus on title
        self.title_entry.focus()
    
    def save_snippet(self):
        """Save the current snippet."""
        title = self.title_entry.get().strip()
        if not title:
            messagebox.showwarning("Validation", "Please enter a title")
            return
        
        code = self.code_text.get(1.0, tk.END).strip()
        if not code:
            messagebox.showwarning("Validation", "Please enter code")
            return
        
        language = self.language_entry.get().strip()
        if not language:
            messagebox.showwarning("Validation", "Please enter a language")
            return
        
        # Create snippet
        snippet = CodeSnippet(
            id="",  # Will be assigned by client
            title=title,
            description=self.description_entry.get().strip(),
            code=code,
            language=language,
            framework=self.framework_entry.get().strip() or None,
            tags=[t.strip() for t in self.tags_entry.get().split(',') if t.strip()]
        )
        
        try:
            # Index and save
            self.code_indexer.index_snippet(snippet)
            messagebox.showinfo("Success", "Snippet saved successfully!")
            self.refresh_snippets()
            self.new_snippet()
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save snippet: {e}")
    
    def delete_snippet(self):
        """Delete selected snippet."""
        selection = self.snippets_listbox.curselection()
        if not selection:
            messagebox.showwarning("Selection", "Please select a snippet to delete")
            return
        
        if messagebox.askyesno("Confirm", "Are you sure you want to delete this snippet?"):
            index = selection[0]
            snippets = getattr(self.snippets_listbox, 'snippet_data', [])
            if index < len(snippets):
                snippet = snippets[index]
                self.client.delete_snippet(snippet.id)
                self.refresh_snippets()
                self.new_snippet()
    
    def perform_search(self):
        """Perform semantic search."""
        query = self.search_entry.get().strip()
        if not query:
            return
        
        self.search_results.delete(1.0, tk.END)
        self.search_results.insert(tk.END, f"Searching for: {query}\n\n")
        
        try:
            results = self.code_indexer.search_by_description(query, limit=10)
            
            if not results:
                self.search_results.insert(tk.END, "No results found.\n")
                return
            
            for i, result in enumerate(results, 1):
                snippet = result.get('snippet', result)
                score = result.get('score', 0.0)
                
                self.search_results.insert(tk.END, f"{i}. {snippet.title}\n")
                self.search_results.insert(tk.END, f"   Language: {snippet.language}")
                if hasattr(snippet, 'framework') and snippet.framework:
                    self.search_results.insert(tk.END, f", Framework: {snippet.framework}")
                self.search_results.insert(tk.END, f"\n   Similarity: {score:.2%}\n")
                self.search_results.insert(tk.END, f"   {snippet.code[:100]}...\n\n")
        
        except Exception as e:
            self.search_results.insert(tk.END, f"Error: {e}\n")
    
    def start_scraping(self):
        """Start a scraping job."""
        url = self.scraper_url.get().strip()
        if not url:
            messagebox.showwarning("Validation", "Please enter a URL")
            return
        
        scraper_type = self.scraper_type.get()
        job_type_map = {
            'GitHub Repository': 'github_repo',
            'Stack Overflow': 'stackoverflow',
            'Documentation': 'docs'
        }
        job_type = job_type_map.get(scraper_type, 'github_repo')
        
        try:
            max_files = int(self.max_files.get())
        except ValueError:
            max_files = 100
        
        config = ScrapingJobConfig(max_files=max_files)
        
        self.scraper_log.insert(tk.END, f"Starting scraping job...\n")
        self.scraper_log.insert(tk.END, f"Type: {scraper_type}\n")
        self.scraper_log.insert(tk.END, f"URL: {url}\n\n")
        
        try:
            job = self.scraper_manager.create_job(job_type, url, config)
            job = self.scraper_manager.execute_job(job)
            
            self.scraper_log.insert(tk.END, f"Job Status: {job.status.value}\n")
            self.scraper_log.insert(tk.END, f"Snippets Created: {job.results.snippets_created}\n")
            self.scraper_log.insert(tk.END, f"Duplicates Found: {job.results.duplicates_found}\n")
            self.scraper_log.insert(tk.END, f"Errors: {job.results.errors}\n\n")
            
            if job.error_log:
                self.scraper_log.insert(tk.END, "Errors:\n")
                for error in job.error_log:
                    self.scraper_log.insert(tk.END, f"  - {error}\n")
            
            self.refresh_snippets()
        
        except Exception as e:
            self.scraper_log.insert(tk.END, f"Error: {e}\n")
    
    def refresh_statistics(self):
        """Refresh statistics display."""
        self.stats_text.delete(1.0, tk.END)
        
        stats = self.client.get_statistics()
        
        self.stats_text.insert(tk.END, "=== ThemisDB Coding Platform Statistics ===\n\n")
        self.stats_text.insert(tk.END, f"Total Snippets: {stats['total_snippets']}\n")
        self.stats_text.insert(tk.END, f"Total Projects: {stats['total_projects']}\n")
        self.stats_text.insert(tk.END, f"Total Documentation: {stats['total_docs']}\n\n")
        
        self.stats_text.insert(tk.END, "Languages:\n")
        for lang, count in sorted(stats['languages'].items(), key=lambda x: x[1], reverse=True):
            self.stats_text.insert(tk.END, f"  {lang}: {count}\n")
    
    def import_file(self):
        """Import code from file."""
        messagebox.showinfo("Coming Soon", "File import feature coming soon!")
    
    def show_about(self):
        """Show about dialog."""
        messagebox.showinfo(
            "About",
            "ThemisDB Coding Platform\n\n"
            "A comprehensive code management system with:\n"
            "- Semantic code search\n"
            "- Web scraping integration\n"
            "- VSCode extension\n\n"
            "Built with ThemisDB Multi-Model Database"
        )


def main():
    """Main entry point."""
    root = tk.Tk()
    app = CodingPlatformApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
