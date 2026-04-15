"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:28                                ║
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
Vector Search & RAG - Dokumenten-Suche mit Embeddings
"""
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from typing import Optional, List
from models import Document, SearchResult, Collection
from themis_client import VectorSearchClient


class VectorSearchApp:
    """
    Hauptanwendung für Vector Search
    """
    
    def __init__(self, root: tk.Tk):
        """Initialisiert die Anwendung"""
        self.root = root
        self.root.title("Vector Search & RAG - ThemisDB")
        self.root.geometry("1200x800")
        
        # Client
        self.client = VectorSearchClient()
        
        # State
        self.current_collection = "default"
        self.selected_doc_id: Optional[str] = None
        
        # Setup UI
        self.setup_menu()
        self.setup_ui()
        
        # Load demo data
        self.load_demo_data()
        
        # Initial refresh
        self.refresh_documents()
        self.refresh_stats()
    
    def setup_menu(self):
        """Erstellt Menüleiste"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File Menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Datei", menu=file_menu)
        file_menu.add_command(label="Neues Dokument", command=self.add_document, 
                             accelerator="Ctrl+N")
        file_menu.add_command(label="Neue Collection", command=self.create_collection,
                             accelerator="Ctrl+Shift+N")
        file_menu.add_separator()
        file_menu.add_command(label="Aktualisieren", command=self.refresh_all, 
                             accelerator="F5")
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self.root.quit)
        
        # Actions Menu
        actions_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Aktionen", menu=actions_menu)
        actions_menu.add_command(label="Ähnliche finden", command=self.find_similar,
                                accelerator="Ctrl+F")
        
        # Keyboard shortcuts
        self.root.bind('<Control-n>', lambda e: self.add_document())
        self.root.bind('<Control-N>', lambda e: self.create_collection())
        self.root.bind('<F5>', lambda e: self.refresh_all())
        self.root.bind('<Control-f>', lambda e: self.find_similar())
    
    def setup_ui(self):
        """Erstellt die Benutzeroberfläche"""
        # Main container with PanedWindow
        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left Panel: Search + Documents
        left_panel = ttk.Frame(paned)
        paned.add(left_panel, weight=1)
        
        # Search Frame
        search_frame = ttk.LabelFrame(left_panel, text="🔍 Suche", padding=10)
        search_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Collection selector
        ttk.Label(search_frame, text="Collection:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.collection_var = tk.StringVar(value="default")
        self.collection_combo = ttk.Combobox(search_frame, textvariable=self.collection_var,
                                            state='readonly', width=20)
        self.collection_combo.grid(row=0, column=1, sticky=tk.EW, pady=2)
        self.collection_combo.bind('<<ComboboxSelected>>', lambda e: self.refresh_documents())
        
        # Search query
        ttk.Label(search_frame, text="Query:").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.search_var = tk.StringVar()
        self.search_entry = ttk.Entry(search_frame, textvariable=self.search_var)
        self.search_entry.grid(row=1, column=1, sticky=tk.EW, pady=2)
        self.search_entry.bind('<Return>', lambda e: self.perform_search())
        
        # Search buttons
        btn_frame = ttk.Frame(search_frame)
        btn_frame.grid(row=2, column=0, columnspan=2, pady=5)
        ttk.Button(btn_frame, text="🔍 Vector Search", 
                  command=self.perform_search).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="🔀 Hybrid Search",
                  command=self.perform_hybrid_search).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="📋 Alle Anzeigen",
                  command=self.refresh_documents).pack(side=tk.LEFT, padx=2)
        
        search_frame.columnconfigure(1, weight=1)
        
        # Documents List
        docs_frame = ttk.LabelFrame(left_panel, text="📚 Dokumente", padding=10)
        docs_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Treeview for documents
        columns = ('title', 'collection', 'score', 'preview')
        self.docs_tree = ttk.Treeview(docs_frame, columns=columns, show='tree headings', height=15)
        
        self.docs_tree.heading('#0', text='ID')
        self.docs_tree.heading('title', text='Titel')
        self.docs_tree.heading('collection', text='Collection')
        self.docs_tree.heading('score', text='Score')
        self.docs_tree.heading('preview', text='Vorschau')
        
        self.docs_tree.column('#0', width=100)
        self.docs_tree.column('title', width=200)
        self.docs_tree.column('collection', width=100)
        self.docs_tree.column('score', width=60)
        self.docs_tree.column('preview', width=300)
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(docs_frame, orient=tk.VERTICAL, command=self.docs_tree.yview)
        self.docs_tree.configure(yscrollcommand=scrollbar.set)
        
        self.docs_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Bind selection
        self.docs_tree.bind('<<TreeviewSelect>>', self.on_document_selected)
        
        # Right Panel: Document Details + Stats
        right_panel = ttk.Frame(paned)
        paned.add(right_panel, weight=1)
        
        # Document Details
        details_frame = ttk.LabelFrame(right_panel, text="📄 Dokument-Details", padding=10)
        details_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Title
        ttk.Label(details_frame, text="Titel:").pack(anchor=tk.W)
        self.detail_title = tk.StringVar()
        title_entry = ttk.Entry(details_frame, textvariable=self.detail_title, state='readonly')
        title_entry.pack(fill=tk.X, pady=(0, 10))
        
        # Content
        ttk.Label(details_frame, text="Inhalt:").pack(anchor=tk.W)
        self.detail_content = scrolledtext.ScrolledText(details_frame, wrap=tk.WORD, 
                                                        height=20, state='disabled')
        self.detail_content.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # Metadata
        ttk.Label(details_frame, text="Metadata:").pack(anchor=tk.W)
        self.detail_metadata = scrolledtext.ScrolledText(details_frame, wrap=tk.WORD,
                                                         height=5, state='disabled')
        self.detail_metadata.pack(fill=tk.BOTH, pady=(0, 10))
        
        # Action buttons
        btn_frame = ttk.Frame(details_frame)
        btn_frame.pack(fill=tk.X)
        ttk.Button(btn_frame, text="✏️ Bearbeiten", 
                  command=self.edit_document).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="🗑️ Löschen",
                  command=self.delete_document).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="🔍 Ähnliche",
                  command=self.find_similar).pack(side=tk.LEFT, padx=2)
        
        # Statistics
        stats_frame = ttk.LabelFrame(right_panel, text="📊 Statistiken", padding=10)
        stats_frame.pack(fill=tk.X, padx=5, pady=5)
        
        self.stats_text = tk.Text(stats_frame, height=6, state='disabled', wrap=tk.WORD)
        self.stats_text.pack(fill=tk.BOTH, expand=True)
    
    def load_demo_data(self):
        """Lädt Demo-Daten"""
        # Create default collection
        self.client.create_collection("default", "Standard-Dokumente")
        self.client.create_collection("technical", "Technische Dokumentation")
        
        # Demo documents
        demo_docs = [
            {
                "title": "Einführung in ThemisDB",
                "content": "ThemisDB ist eine moderne, flexible Datenbank für verschiedene Datenmodelle. "
                          "Sie unterstützt relationale Daten, Graphen, Zeitreihen und Vector Search. "
                          "Die API ist einfach zu verwenden und bietet hohe Performance.",
                "collection": "default",
                "metadata": {"author": "ThemisDB Team", "type": "introduction"}
            },
            {
                "title": "Vector Search Tutorial",
                "content": "Vector Search ermöglicht semantische Suche durch Embeddings. "
                          "Texte werden in hochdimensionale Vektoren umgewandelt. "
                          "Die Ähnlichkeit wird durch Cosine Similarity berechnet. "
                          "Dies erlaubt Suche nach Bedeutung statt Keywords.",
                "collection": "technical",
                "metadata": {"author": "AI Team", "type": "tutorial"}
            },
            {
                "title": "Python API Guide",
                "content": "Die Python API für ThemisDB ist einfach und intuitiv. "
                          "Sie können CRUD-Operationen, Queries und Transaktionen durchführen. "
                          "Die Client-Bibliothek kümmert sich um Connection Management und Retry Logic.",
                "collection": "technical",
                "metadata": {"author": "Dev Team", "type": "guide"}
            },
            {
                "title": "Machine Learning Integration",
                "content": "ThemisDB integriert sich nahtlos mit ML-Frameworks. "
                          "Sie können Embeddings von Hugging Face Modellen verwenden. "
                          "Die Datenbank speichert Vektoren effizient und bietet schnelle Ähnlichkeitssuche.",
                "collection": "default",
                "metadata": {"author": "ML Team", "type": "integration"}
            },
            {
                "title": "RAG Pattern Explained",
                "content": "Retrieval Augmented Generation kombiniert Informationsretrieval mit LLMs. "
                          "Zuerst werden relevante Dokumente gesucht, dann als Context für das LLM verwendet. "
                          "Dies verbessert Antworten durch aktuelle und spezifische Informationen.",
                "collection": "technical",
                "metadata": {"author": "AI Team", "type": "pattern"}
            }
        ]
        
        for doc_data in demo_docs:
            doc_id = Document.create_id(doc_data["title"], doc_data["content"])
            document = Document(
                id=doc_id,
                title=doc_data["title"],
                content=doc_data["content"],
                collection=doc_data["collection"],
                metadata=doc_data["metadata"]
            )
            self.client.add_document(document, auto_embed=True)
        
        # Update collection selector
        self.update_collection_selector()
    
    def update_collection_selector(self):
        """Aktualisiert Collection Selector"""
        collections = self.client.list_collections()
        collection_names = [col.name for col in collections]
        self.collection_combo['values'] = collection_names
        if collection_names and not self.collection_var.get():
            self.collection_var.set(collection_names[0])
    
    def refresh_documents(self, results: Optional[List[SearchResult]] = None):
        """Aktualisiert Dokumentenliste"""
        # Clear tree
        for item in self.docs_tree.get_children():
            self.docs_tree.delete(item)
        
        # Get documents
        if results is None:
            collection = self.collection_var.get() if self.collection_var.get() else None
            documents = self.client.list_documents(collection)
            # Convert to SearchResults without scores
            results = [SearchResult(document=doc, score=0.0) for doc in documents]
        
        # Populate tree
        for result in results:
            doc = result.document
            score_str = f"{result.score:.3f}" if result.score > 0 else "-"
            preview = doc.get_preview(100)
            
            self.docs_tree.insert('', tk.END, text=doc.id[:8] + "...",
                                values=(doc.title, doc.collection, score_str, preview))
    
    def refresh_stats(self):
        """Aktualisiert Statistiken"""
        stats = self.client.get_stats()
        
        stats_text = f"""
📊 Statistiken:
━━━━━━━━━━━━━━━━━━━━━━━━━━━
📚 Dokumente: {stats['total_documents']}
🔢 Mit Embeddings: {stats['documents_with_embeddings']}
📁 Collections: {stats['total_collections']}
🤖 Modell: {stats['embedding_model']}
📐 Dimensionen: {stats['embedding_dimension']}
        """.strip()
        
        self.stats_text.configure(state='normal')
        self.stats_text.delete(1.0, tk.END)
        self.stats_text.insert(1.0, stats_text)
        self.stats_text.configure(state='disabled')
    
    def refresh_all(self):
        """Aktualisiert alles"""
        self.refresh_documents()
        self.refresh_stats()
        self.update_collection_selector()
    
    def on_document_selected(self, event):
        """Callback für Dokument-Auswahl"""
        selection = self.docs_tree.selection()
        if not selection:
            return
        
        item = selection[0]
        doc_id_short = self.docs_tree.item(item, 'text')
        
        # Find full document
        for doc_id, doc in self.client.documents.items():
            if doc_id.startswith(doc_id_short.replace("...", "")):
                self.selected_doc_id = doc_id
                self.show_document_details(doc)
                break
    
    def show_document_details(self, document: Document):
        """Zeigt Dokument-Details"""
        self.detail_title.set(document.title)
        
        # Content
        self.detail_content.configure(state='normal')
        self.detail_content.delete(1.0, tk.END)
        self.detail_content.insert(1.0, document.content)
        self.detail_content.configure(state='disabled')
        
        # Metadata
        metadata_str = f"Collection: {document.collection}\n"
        metadata_str += f"Erstellt: {document.created_at}\n"
        metadata_str += f"Embedding: {'Ja' if document.embedding else 'Nein'}\n"
        if document.metadata:
            metadata_str += "\nZusätzliche Metadaten:\n"
            for key, value in document.metadata.items():
                metadata_str += f"  {key}: {value}\n"
        
        self.detail_metadata.configure(state='normal')
        self.detail_metadata.delete(1.0, tk.END)
        self.detail_metadata.insert(1.0, metadata_str)
        self.detail_metadata.configure(state='disabled')
    
    def perform_search(self):
        """Führt Vector Search aus"""
        query = self.search_var.get().strip()
        if not query:
            messagebox.showwarning("Warnung", "Bitte Query eingeben")
            return
        
        collection = self.collection_var.get() if self.collection_var.get() else None
        results = self.client.search(query, collection, top_k=20)
        
        if not results:
            messagebox.showinfo("Info", "Keine Ergebnisse gefunden")
            return
        
        self.refresh_documents(results)
        messagebox.showinfo("Suche", f"{len(results)} Ergebnisse gefunden")
    
    def perform_hybrid_search(self):
        """Führt Hybrid Search aus"""
        query = self.search_var.get().strip()
        if not query:
            messagebox.showwarning("Warnung", "Bitte Query eingeben")
            return
        
        collection = self.collection_var.get() if self.collection_var.get() else None
        results = self.client.search_hybrid(query, collection, top_k=20)
        
        if not results:
            messagebox.showinfo("Info", "Keine Ergebnisse gefunden")
            return
        
        self.refresh_documents(results)
        messagebox.showinfo("Suche", f"{len(results)} Ergebnisse gefunden (Hybrid)")
    
    def find_similar(self):
        """Findet ähnliche Dokumente"""
        if not self.selected_doc_id:
            messagebox.showwarning("Warnung", "Bitte Dokument auswählen")
            return
        
        results = self.client.find_similar(self.selected_doc_id, top_k=10)
        
        if not results:
            messagebox.showinfo("Info", "Keine ähnlichen Dokumente gefunden")
            return
        
        self.refresh_documents(results)
        messagebox.showinfo("Ähnliche Dokumente", f"{len(results)} ähnliche Dokumente gefunden")
    
    def add_document(self):
        """Fügt neues Dokument hinzu"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Neues Dokument")
        dialog.geometry("600x500")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Title
        ttk.Label(dialog, text="Titel:").pack(anchor=tk.W, padx=10, pady=(10, 0))
        title_var = tk.StringVar()
        ttk.Entry(dialog, textvariable=title_var).pack(fill=tk.X, padx=10, pady=5)
        
        # Collection
        ttk.Label(dialog, text="Collection:").pack(anchor=tk.W, padx=10)
        collection_var = tk.StringVar(value=self.collection_var.get())
        ttk.Combobox(dialog, textvariable=collection_var,
                    values=[col.name for col in self.client.list_collections()]).pack(fill=tk.X, padx=10, pady=5)
        
        # Content
        ttk.Label(dialog, text="Inhalt:").pack(anchor=tk.W, padx=10)
        content_text = scrolledtext.ScrolledText(dialog, wrap=tk.WORD, height=15)
        content_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        def save():
            title = title_var.get().strip()
            content = content_text.get(1.0, tk.END).strip()
            collection = collection_var.get().strip()
            
            if not title or not content:
                messagebox.showwarning("Warnung", "Titel und Inhalt erforderlich")
                return
            
            doc_id = Document.create_id(title, content)
            document = Document(id=doc_id, title=title, content=content, collection=collection)
            self.client.add_document(document, auto_embed=True)
            
            messagebox.showinfo("Erfolg", "Dokument hinzugefügt")
            dialog.destroy()
            self.refresh_all()
        
        ttk.Button(dialog, text="💾 Speichern", command=save).pack(pady=10)
    
    def edit_document(self):
        """Bearbeitet Dokument"""
        if not self.selected_doc_id:
            messagebox.showwarning("Warnung", "Bitte Dokument auswählen")
            return
        
        document = self.client.get_document(self.selected_doc_id)
        if not document:
            return
        
        # Similar to add_document but with pre-filled values
        dialog = tk.Toplevel(self.root)
        dialog.title("Dokument bearbeiten")
        dialog.geometry("600x500")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Title
        ttk.Label(dialog, text="Titel:").pack(anchor=tk.W, padx=10, pady=(10, 0))
        title_var = tk.StringVar(value=document.title)
        ttk.Entry(dialog, textvariable=title_var).pack(fill=tk.X, padx=10, pady=5)
        
        # Collection
        ttk.Label(dialog, text="Collection:").pack(anchor=tk.W, padx=10)
        collection_var = tk.StringVar(value=document.collection)
        ttk.Combobox(dialog, textvariable=collection_var,
                    values=[col.name for col in self.client.list_collections()]).pack(fill=tk.X, padx=10, pady=5)
        
        # Content
        ttk.Label(dialog, text="Inhalt:").pack(anchor=tk.W, padx=10)
        content_text = scrolledtext.ScrolledText(dialog, wrap=tk.WORD, height=15)
        content_text.insert(1.0, document.content)
        content_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        def save():
            title = title_var.get().strip()
            content = content_text.get(1.0, tk.END).strip()
            collection = collection_var.get().strip()
            
            if not title or not content:
                messagebox.showwarning("Warnung", "Titel und Inhalt erforderlich")
                return
            
            self.client.update_document(self.selected_doc_id, 
                                       title=title, content=content, collection=collection)
            
            messagebox.showinfo("Erfolg", "Dokument aktualisiert")
            dialog.destroy()
            self.refresh_all()
            
            # Refresh details view
            updated_doc = self.client.get_document(self.selected_doc_id)
            if updated_doc:
                self.show_document_details(updated_doc)
        
        ttk.Button(dialog, text="💾 Speichern", command=save).pack(pady=10)
    
    def delete_document(self):
        """Löscht Dokument"""
        if not self.selected_doc_id:
            messagebox.showwarning("Warnung", "Bitte Dokument auswählen")
            return
        
        document = self.client.get_document(self.selected_doc_id)
        if not document:
            return
        
        if messagebox.askyesno("Bestätigung", 
                              f"Dokument '{document.title}' wirklich löschen?"):
            self.client.delete_document(self.selected_doc_id)
            self.selected_doc_id = None
            messagebox.showinfo("Erfolg", "Dokument gelöscht")
            self.refresh_all()
            
            # Clear details
            self.detail_title.set("")
            self.detail_content.configure(state='normal')
            self.detail_content.delete(1.0, tk.END)
            self.detail_content.configure(state='disabled')
            self.detail_metadata.configure(state='normal')
            self.detail_metadata.delete(1.0, tk.END)
            self.detail_metadata.configure(state='disabled')
    
    def create_collection(self):
        """Erstellt neue Collection"""
        dialog = tk.Toplevel(self.root)
        dialog.title("Neue Collection")
        dialog.geometry("400x200")
        dialog.transient(self.root)
        dialog.grab_set()
        
        ttk.Label(dialog, text="Name:").pack(anchor=tk.W, padx=10, pady=(10, 0))
        name_var = tk.StringVar()
        ttk.Entry(dialog, textvariable=name_var).pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Label(dialog, text="Beschreibung:").pack(anchor=tk.W, padx=10)
        desc_var = tk.StringVar()
        ttk.Entry(dialog, textvariable=desc_var).pack(fill=tk.X, padx=10, pady=5)
        
        def save():
            name = name_var.get().strip()
            desc = desc_var.get().strip()
            
            if not name:
                messagebox.showwarning("Warnung", "Name erforderlich")
                return
            
            self.client.create_collection(name, desc)
            messagebox.showinfo("Erfolg", f"Collection '{name}' erstellt")
            dialog.destroy()
            self.update_collection_selector()
        
        ttk.Button(dialog, text="💾 Erstellen", command=save).pack(pady=10)


def main():
    """Hauptfunktion"""
    root = tk.Tk()
    app = VectorSearchApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
