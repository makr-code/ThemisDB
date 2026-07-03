"""Tkinter Visualizer for include/markdown graph, chunks and gaps.

Design: SOC + OOP
- GraphModel: lädt JSON (nodes, edges, chunks, gaps)
- AppController: verbindet Model und View
- VisualizerView: reine GUI-Elemente (Tkinter)

Usage:
  python tools/visualizer_tk.py --graph ai_working/include_graph_tools_scanners_libclang.json

Optional: `networkx` is used for nicer layouts if available.
"""
from __future__ import annotations

import argparse
import json
import math
import threading
import queue
import concurrent.futures
import os
import sys
from dataclasses import dataclass
from typing import Dict, List, Tuple
import io
try:
    from PIL import ImageGrab
    HAS_PIL = True
except Exception:
    HAS_PIL = False
from itertools import combinations

try:
    import tkinter as tk
    from tkinter import ttk, messagebox
except Exception:
    print("Tkinter is required but not available in this environment.")
    raise

try:
    import networkx as nx
    HAS_NX = True
except Exception:
    HAS_NX = False

# simple logger for visualizer
import time
import logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s %(message)s')
import logging
LOG_DIR = os.path.join(os.getcwd(), 'ai_working')
os.makedirs(LOG_DIR, exist_ok=True)
LOG_PATH = os.path.join(LOG_DIR, 'visualizer_log.txt')
logging.basicConfig(filename=LOG_PATH, level=logging.INFO, format='%(asctime)s %(levelname)s: %(message)s')


@dataclass
class GraphNode:
    id: str
    label: str
    meta: Dict


@dataclass
class GraphEdge:
    source: str
    target: str
    meta: Dict


class GraphModel:
    """Lädt Graph, Chunks und Gaps aus JSON-Daten (Pfad oder dict) und stellt einfache API bereit."""

    def __init__(self, path_or_data):
        # path_or_data: either a filesystem path (str) or a pre-loaded dict
        self.path = path_or_data if isinstance(path_or_data, str) else None
        self.nodes: Dict[str, GraphNode] = {}
        self.edges: List[GraphEdge] = []
        self.chunks: Dict[str, List[Dict]] = {}
        self.gaps: Dict[str, List[Dict]] = {}
        self._load(path_or_data)

    def _load(self, path_or_data):
        if isinstance(path_or_data, dict):
            data = path_or_data
        else:
            if not os.path.exists(path_or_data):
                raise FileNotFoundError(path_or_data)
            with open(path_or_data, "r", encoding="utf-8") as f:
                data = json.load(f)

        # Expecting JSON with 'nodes', 'edges', 'chunks', 'gaps' keys (best-effort)
        raw_nodes = data.get("nodes", [])
        for n in raw_nodes:
            nid = n.get("id") or n.get("path") or n.get("name")
            label = n.get("label") or n.get("path") or nid
            self.nodes[str(nid)] = GraphNode(id=str(nid), label=label, meta=n)

        raw_edges = data.get("edges", [])
        for e in raw_edges:
            s = e.get("source") or e.get("from")
            t = e.get("target") or e.get("to")
            if s and t:
                self.edges.append(GraphEdge(source=str(s), target=str(t), meta=e))

        # chunks: mapping file -> list of chunks
        self.chunks = data.get("chunks", {}) or {}
        # gaps: mapping file -> list of gaps
        self.gaps = data.get("gaps", {}) or {}

    def list_nodes(self) -> List[GraphNode]:
        return list(self.nodes.values())

    def neighbors(self, node_id: str) -> List[str]:
        nbrs = [e.target for e in self.edges if e.source == node_id]
        nbrs += [e.source for e in self.edges if e.target == node_id]
        return list(dict.fromkeys(nbrs))


class VisualizerView(tk.Tk):
    """Tkinter GUI. Minimal, keeps presentation only."""

    def __init__(self):
        super().__init__()
        self.title("Graph / Chunks / Gaps Visualizer")
        self.geometry("1100x700")

        self._create_widgets()

    def _create_widgets(self):
        # Menu bar (Windows standard)
        menubar = tk.Menu(self)
        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Open...", command=lambda: getattr(self, '_on_file_open', lambda: None)(), accelerator="Ctrl+O")
        file_menu.add_command(label="Save Session...", command=lambda: getattr(self, '_on_save_session', lambda: None)())
        file_menu.add_command(label="Load Session...", command=lambda: getattr(self, '_on_load_session', lambda: None)())
        file_menu.add_command(label="Export PNG...", command=lambda: getattr(self, '_on_export_png', lambda: None)())
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.quit, accelerator="Alt+F4")
        menubar.add_cascade(label="File", menu=file_menu)

        view_menu = tk.Menu(menubar, tearoff=0)
        view_menu.add_command(label="Refresh", command=lambda: getattr(self, '_on_refresh', lambda: None)(), accelerator="F5")
        view_menu.add_command(label="Fit to view", command=lambda: getattr(self, '_on_fit', lambda: None)())
        view_menu.add_checkbutton(label="Use networkx layout", command=lambda: getattr(self, '_on_toggle_networkx', lambda: None)())
        menubar.add_cascade(label="View", menu=view_menu)

        help_menu = tk.Menu(menubar, tearoff=0)
        help_menu.add_command(label="About", command=lambda: getattr(self, '_on_about', lambda: None)())
        menubar.add_cascade(label="Help", menu=help_menu)

        self.config(menu=menubar)

        # Top toolbar: quick actions + search/filters
        toolbar = ttk.Frame(self)
        toolbar.pack(fill=tk.X, padx=6, pady=4)

        self.open_btn = ttk.Button(toolbar, text="Open", width=8, command=lambda: getattr(self, '_on_file_open', lambda: None)())
        self.open_btn.pack(side=tk.LEFT, padx=(0, 6))
        self.refresh_button = ttk.Button(toolbar, text="Refresh", width=8, command=lambda: getattr(self, '_on_refresh', lambda: None)())
        self.refresh_button.pack(side=tk.LEFT)
        self.zoom_in_btn = ttk.Button(toolbar, text="Zoom+", width=6)
        self.zoom_in_btn.pack(side=tk.LEFT, padx=(6, 2))
        self.zoom_out_btn = ttk.Button(toolbar, text="Zoom-", width=6)
        self.zoom_out_btn.pack(side=tk.LEFT, padx=(2, 8))
        self.fit_btn = ttk.Button(toolbar, text="Fit", width=6, command=lambda: getattr(self, '_on_fit', lambda: None)())
        self.fit_btn.pack(side=tk.LEFT)

        ttk.Label(toolbar, text="Mode:").pack(side=tk.LEFT, padx=(12, 2))
        self.mode_var = tk.StringVar(value="force")
        self.mode_cb = ttk.Combobox(toolbar, textvariable=self.mode_var, state="readonly", width=12)
        self.mode_cb['values'] = ["force", "galaxy"]
        self.mode_cb.pack(side=tk.LEFT, padx=(4, 8))
        
        # clustering and heatmap toggles
        self.clustering_var = tk.BooleanVar(value=False)
        self.heatmap_var = tk.BooleanVar(value=False)
        self.cluster_cb = ttk.Checkbutton(toolbar, text="Clustering", variable=self.clustering_var, command=lambda: getattr(self, '_on_toggle_cluster', lambda: None)())
        self.cluster_cb.pack(side=tk.LEFT, padx=(6, 2))
        self.heatmap_cb = ttk.Checkbutton(toolbar, text="Heatmap", variable=self.heatmap_var, command=lambda: getattr(self, '_on_toggle_heatmap', lambda: None)())
        self.heatmap_cb.pack(side=tk.LEFT, padx=(2, 8))

        ttk.Label(toolbar, text="Cluster by:").pack(side=tk.LEFT, padx=(6,2))
        self.cluster_key_var = tk.StringVar(value="topdir")
        self.cluster_key_cb = ttk.Combobox(toolbar, textvariable=self.cluster_key_var, state="readonly", width=14)
        self.cluster_key_cb['values'] = ["topdir", "subsystem", "scanner", "none"]
        self.cluster_key_cb.pack(side=tk.LEFT, padx=(4,8))
        # cluster count display
        try:
            self.cluster_count_var = tk.StringVar(value="Clusters: 0")
            self.cluster_count_label = ttk.Label(toolbar, textvariable=self.cluster_count_var)
            self.cluster_count_label.pack(side=tk.LEFT, padx=(6,8))
        except Exception:
            self.cluster_count_var = None

        # Legend: small heatmap preview and node size control
        try:
            self.legend_canvas = tk.Canvas(toolbar, width=120, height=16)
            # draw simple gradient
            for i in range(120):
                s = i / 119.0
                r = int(55 + 200 * s)
                g = int(55 + 200 * (1.0 - s))
                b = 80
                color = f"#{r:02x}{g:02x}{b:02x}"
                self.legend_canvas.create_line(i, 0, i, 16, fill=color)
            self.legend_canvas.pack(side=tk.LEFT, padx=(6,4))
        except Exception:
            self.legend_canvas = None

        # Grid / rulers controls
        self.grid_var = tk.BooleanVar(value=True)
        try:
            self.grid_cb = ttk.Checkbutton(toolbar, text="Grid", variable=self.grid_var, command=lambda: getattr(self, '_on_toggle_grid', lambda: None)())
            self.grid_cb.pack(side=tk.LEFT, padx=(6,2))
        except Exception:
            self.grid_cb = None
        self.grid_spacing_var = tk.IntVar(value=50)
        try:
            self.grid_spacing_spin = ttk.Spinbox(toolbar, from_=10, to=500, increment=10, textvariable=self.grid_spacing_var, width=6)
            self.grid_spacing_spin.pack(side=tk.LEFT, padx=(2,8))
        except Exception:
            self.grid_spacing_spin = None
        # layout iterations control
        self.layout_max_iter_var = tk.IntVar(value=40)
        try:
            ttk.Label(toolbar, text="Max iters:").pack(side=tk.LEFT)
            self.layout_max_iter_spin = ttk.Spinbox(toolbar, from_=5, to=500, increment=1, textvariable=self.layout_max_iter_var, width=6)
            self.layout_max_iter_spin.pack(side=tk.LEFT, padx=(2,8))
        except Exception:
            self.layout_max_iter_spin = None

        # Color map selector for heatmap
        ttk.Label(toolbar, text="Map:").pack(side=tk.LEFT)
        self.color_map_var = tk.StringVar(value="rg")
        try:
            self.color_map_cb = ttk.Combobox(toolbar, textvariable=self.color_map_var, state="readonly", width=6)
            self.color_map_cb['values'] = ["rg", "br", "viridis"]
            self.color_map_cb.pack(side=tk.LEFT, padx=(2,6))
            self.color_map_cb.bind('<<ComboboxSelected>>', lambda e: self._update_legend_preview())
        except Exception:
            self.color_map_cb = None

        ttk.Label(toolbar, text="Size:").pack(side=tk.LEFT)
        self.node_size_var = tk.DoubleVar(value=1.0)
        try:
            self.node_size_slider = ttk.Scale(toolbar, from_=0.5, to=3.0, orient=tk.HORIZONTAL, variable=self.node_size_var, length=100)
            self.node_size_slider.pack(side=tk.LEFT, padx=(2,8))
        except Exception:
            self.node_size_slider = None

        # ensure legend updates when slider or map changes
        try:
            if self.legend_canvas:
                self._update_legend_preview()
        except Exception:
            pass

        ttk.Label(toolbar, text="Search:").pack(side=tk.LEFT, padx=(12, 2))
        self.search_var = tk.StringVar()
        self.search_entry = ttk.Entry(toolbar, textvariable=self.search_var, width=30)
        self.search_entry.pack(side=tk.LEFT, padx=(4, 8))

        ttk.Label(toolbar, text="Filter gap:").pack(side=tk.LEFT)
        self.filter_gap_var = tk.StringVar()
        self.filter_gap_cb = ttk.Combobox(toolbar, textvariable=self.filter_gap_var, state="readonly", width=20)
        self.filter_gap_cb.pack(side=tk.LEFT, padx=(4, 8))
        self.filter_gap_cb['values'] = ["(any)"]

        ttk.Label(toolbar, text="Sort:").pack(side=tk.LEFT)
        self.sort_var = tk.StringVar(value="label")
        self.sort_cb = ttk.Combobox(toolbar, textvariable=self.sort_var, state="readonly", width=14)
        self.sort_cb['values'] = ["label", "gap_count", "chunk_count"]
        self.sort_cb.pack(side=tk.LEFT, padx=(4, 8))

        # Main layout: paned with left/right sidebars and center content (tabs)
        paned = ttk.Panedwindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True)

        # Left sidebar with tabs (Nodes, Filters)
        left_notebook = ttk.Notebook(paned, width=280)
        left_frame_nodes = ttk.Frame(left_notebook)
        left_frame_filters = ttk.Frame(left_notebook)
        left_notebook.add(left_frame_nodes, text="Nodes")
        left_notebook.add(left_frame_filters, text="Filters")

        # Center content with tabs (Graph, Raw JSON)
        center_notebook = ttk.Notebook(paned)
        center_frame_graph = ttk.Frame(center_notebook)
        center_frame_raw = ttk.Frame(center_notebook)
        center_notebook.add(center_frame_graph, text="Graph")
        center_notebook.add(center_frame_raw, text="Raw JSON")

        # Right sidebar with tabs (Chunks, Gaps)
        right_notebook = ttk.Notebook(paned, width=360)
        right_frame_chunks = ttk.Frame(right_notebook)
        right_frame_gaps = ttk.Frame(right_notebook)
        right_notebook.add(right_frame_chunks, text="Chunks")
        right_notebook.add(right_frame_gaps, text="Gaps")

        paned.add(left_notebook, weight=1)
        paned.add(center_notebook, weight=3)
        paned.add(right_notebook, weight=1)

        # Left: nodes tree (manipulable)
        self.node_list = ttk.Treeview(left_frame_nodes, columns=("gaps",), show='tree')
        self.node_list.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)

        # Filters tab: advanced filters (multi-select gap types, severity, regex)
        self.filters_frame = ttk.Frame(left_frame_filters)
        self.filters_frame.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)
        ttk.Label(self.filters_frame, text="Gap Types:").pack(anchor=tk.W)
        self.filter_gap_listbox = tk.Listbox(self.filters_frame, selectmode=tk.MULTIPLE, height=8)
        self.filter_gap_listbox.pack(fill=tk.BOTH, expand=False, padx=2, pady=4)
        try:
            self.filter_gap_listbox.bind('<<ListboxSelect>>', lambda e: self.apply_filters())
        except Exception:
            pass

        ttk.Label(self.filters_frame, text="Severity:").pack(anchor=tk.W, pady=(6, 0))
        self.filter_severity_var = tk.StringVar(value="(any)")
        self.filter_severity_cb = ttk.Combobox(self.filters_frame, textvariable=self.filter_severity_var, state="readonly", width=18)
        self.filter_severity_cb['values'] = ["(any)", "CRITICAL", "HIGH", "MEDIUM", "LOW"]
        self.filter_severity_cb.pack(anchor=tk.W, padx=2, pady=4)

        self.regex_var = tk.BooleanVar(value=False)
        self.regex_cb = ttk.Checkbutton(self.filters_frame, text="Regex search", variable=self.regex_var)
        self.regex_cb.pack(anchor=tk.W, padx=2, pady=(6, 2))

        ttk.Label(self.filters_frame, text="Cluster Keys:").pack(anchor=tk.W, pady=(8,2))
        self.cluster_listbox = tk.Listbox(self.filters_frame, selectmode=tk.SINGLE, height=6)
        self.cluster_listbox.pack(fill=tk.BOTH, expand=False, padx=2, pady=4)
        try:
            self.cluster_listbox.bind('<<ListboxSelect>>', lambda e: getattr(self, '_on_cluster_list_select', lambda ev: None)(e))
        except Exception:
            pass

        ttk.Label(self.filters_frame, text="Export").pack(anchor=tk.W, pady=(8, 2))
        exp_frame = ttk.Frame(self.filters_frame)
        exp_frame.pack(fill=tk.X)
        self.export_json_btn = ttk.Button(exp_frame, text="Export Visible JSON", command=lambda: getattr(self, 'export_visible_json', lambda: None)())
        self.export_json_btn.pack(side=tk.LEFT, padx=2)
        self.export_csv_btn = ttk.Button(exp_frame, text="Export Visible CSV", command=lambda: getattr(self, 'export_visible_csv', lambda: None)())
        self.export_csv_btn.pack(side=tk.LEFT, padx=2)

        # Center Graph canvas with scrollbars (infinite content window)
        canvas_container = ttk.Frame(center_frame_graph)
        canvas_container.pack(fill=tk.BOTH, expand=True)

        self.hbar = ttk.Scrollbar(canvas_container, orient=tk.HORIZONTAL)
        self.vbar = ttk.Scrollbar(canvas_container, orient=tk.VERTICAL)
        self.canvas = tk.Canvas(canvas_container, bg="white", xscrollcommand=self.hbar.set, yscrollcommand=self.vbar.set)
        self.hbar.config(command=self.canvas.xview)
        self.vbar.config(command=self.canvas.yview)

        # layout: canvas fills, vbar on right, hbar on bottom
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.vbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.hbar.pack(side=tk.BOTTOM, fill=tk.X)

        # initialize an effectively infinite scrollregion centered at origin
        try:
            self.canvas.configure(scrollregion=(-10000, -10000, 10000, 10000))
        except Exception:
            pass

        # Center Raw JSON view
        self.raw_text = tk.Text(center_frame_raw)
        self.raw_text.pack(fill=tk.BOTH, expand=True)

        # Right: chunks and gaps
        # Right: chunks and gaps as Treeviews for manipulation
        self.chunks_tree = ttk.Treeview(right_frame_chunks, columns=("start","text"), show='tree')
        self.chunks_tree.heading('#0', text='Chunk')
        self.chunks_tree.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)
        self.gaps_tree = ttk.Treeview(right_frame_gaps, columns=("type", "severity", "desc"), show='tree')
        self.gaps_tree.heading('#0', text='Gap')
        self.gaps_tree.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)

        # Status bar
        status_frame = ttk.Frame(self)
        status_frame.pack(fill=tk.X)
        self.status = ttk.Label(status_frame, text="Ready", anchor=tk.W)
        self.status.pack(side=tk.LEFT, fill=tk.X, expand=True)
        try:
            self.progress = ttk.Progressbar(status_frame, orient=tk.HORIZONTAL, mode='determinate', length=180)
            # hide initially
            self.progress.pack(side=tk.RIGHT, padx=(6,8))
            self.progress.pack_forget()
        except Exception:
            self.progress = None

        # keyboard shortcuts
        self.bind_all('<Control-o>', lambda e: getattr(self, '_on_file_open', lambda: None)())
        self.bind_all('<F5>', lambda e: getattr(self, '_on_refresh', lambda: None)())


class AppController:
    """Connects Model and View, handles events and drawing."""

    NODE_RADIUS = 22
    # LOD thresholds (zoom_factor uses view_w/(bbox_w+1))
    SUPERCLUSTER_ZOOM = 0.06
    CLUSTER_ZOOM = 0.25
    CLUSTER_EXPAND_ZOOM = 0.35
    LARGE_CLUSTER_SIZE = 150

    def __init__(self, model: GraphModel | None, view: VisualizerView):
        self.model = model
        self.view = view
        self.node_positions: Dict[str, Tuple[float, float]] = {}
        self.visible_nodes: List[str] = []
        # animation / rendering state
        self._target_positions: Dict[str, Tuple[float, float]] = {}
        self._anim_frames = 0
        self._anim_total_frames = 10
        self._batch_draw_job = None
        self._batch_start = 0
        self._batch_size = 200
        self._pan_data = None
        self._cluster_map: Dict[str, List[str]] = {}
        # background filter executor + result queue
        self._filter_executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        self._result_queue: "queue.Queue[dict]" = queue.Queue()
        self._filter_lock = threading.Lock()
        # executor for layout computations to avoid blocking UI
        self._layout_executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        self._layout_future = None
        self._layout_lock = threading.Lock()

        # redraw-on-demand state
        self._needs_redraw = False
        self._redraw_job = None
        self._redraw_region = None

        # only setup if model provided; otherwise wait for set_model to be called
        if self.model is not None:
            self._setup()
            try:
                self.view.after(100, self._poll_results)
            except Exception:
                pass

    def _setup(self):
        logging.debug("AppController: setup called")
        # populate node list
        # prepare filter values
        gap_types = set()
        for vals in self.model.gaps.values():
            for g in vals:
                gt = g.get("type") or g.get("gap_type")
                if gt:
                    gap_types.add(gt)

        gap_list = ["(any)"] + sorted(gap_types)
        try:
            self.view.filter_gap_cb['values'] = gap_list
        except Exception:
            pass
        
        # populate advanced multi-select listbox
        try:
            self.view.filter_gap_listbox.delete(0, tk.END)
            for g in sorted(gap_types):
                self.view.filter_gap_listbox.insert(tk.END, g)
        except Exception:
            pass

        # populate cluster listbox (if available)
        try:
            if getattr(self.view, 'cluster_listbox', None):
                self.view.cluster_listbox.delete(0, tk.END)
                for ck, members in sorted(self._cluster_map.items()):
                    self.view.cluster_listbox.insert(tk.END, f"{ck} ({len(members)})")
        except Exception:
            pass

        self.view.refresh_button.config(command=self.apply_filters)
        self.view.search_entry.bind("<KeyRelease>", lambda e: self.apply_filters())
        self.view.sort_cb.bind("<<ComboboxSelected>>", lambda e: self.apply_filters())
        # toolbar zoom handlers
        self.view.zoom_in_btn.config(command=self.zoom_in)
        self.view.zoom_out_btn.config(command=self.zoom_out)

        # menu callbacks exposed via view
        self.view._on_file_open = self.open_file_dialog
        self.view._on_refresh = self.apply_filters
        self.view._on_fit = self.fit_view
        self.view._on_toggle_networkx = self.toggle_networkx
        self.view._on_about = self.show_about
        # grid toggle should request redraw
        try:
            self.view._on_toggle_grid = self.request_redraw
        except Exception:
            pass
        # react to spacing changes
        try:
            gv = getattr(self.view, 'grid_spacing_var', None)
            if gv is not None:
                try:
                    gv.trace_add('write', lambda *args: self.request_redraw())
                except Exception:
                    try:
                        gv.trace_variable('w', lambda *args: self.request_redraw())
                    except Exception:
                        pass
        except Exception:
            pass
        # file menu actions for session and export
        self.view._on_export_png = self.export_canvas_png
        self.view._on_save_session = self.save_session
        self.view._on_load_session = self.load_session
        # canvas interaction: mousewheel zoom and middle-button pan
        try:
            c = self.view.canvas
            c.bind('<MouseWheel>', self._on_canvas_mousewheel)
            c.bind('<Button-4>', self._on_canvas_mousewheel)
            c.bind('<Button-5>', self._on_canvas_mousewheel)
            c.bind('<ButtonPress-2>', self._start_pan)
            c.bind('<B2-Motion>', self._do_pan)
            c.bind('<ButtonRelease-2>', self._end_pan)
        except Exception:
            pass
        self.view.node_list.bind("<<ListboxSelect>>", self.on_node_select)
        # Treeview select event
        try:
            self.view.node_list.unbind("<<ListboxSelect>>")
        except Exception:
            pass
        try:
            self.view.node_list.bind("<<TreeviewSelect>>", self.on_node_select)
        except Exception:
            pass
        # mode/feature bindings
        try:
            self.view.mode_cb.bind("<<ComboboxSelected>>", lambda e: self.apply_filters())
        except Exception:
            pass
        # cluster list selection handler
        try:
            if getattr(self.view, 'cluster_listbox', None):
                self.view._on_cluster_list_select = self._on_cluster_list_select
        except Exception:
            pass
        self.view.canvas.bind("<Configure>", self.on_canvas_resize)

        # canvas interaction state (must exist before drawing)
        self._canvas_node_map = {}  # canvas_item -> node_id (legacy)
        # mappings for reuse: node id -> (oval_id, text_id)
        self._node_canvas_items = {}
        # edge mapping: (source,target) -> line_id
        self._edge_canvas_items = {}
        # cluster aggregates mapping: cluster_key -> (oval_id, text_id)
        self._cluster_canvas_items = {}
        self._tooltip = None

        # initial visible nodes
        self.apply_filters()

    def set_model(self, model: GraphModel):
        """Attach model after controller construction and initialize state."""
        logging.debug("AppController: set_model called")
        self.model = model
        try:
            self._setup()
        except Exception:
            logging.exception("_setup failed in set_model")
        try:
            self.view.after(100, self._poll_results)
        except Exception:
            pass

    def request_redraw(self, immediate: bool = False, region: Tuple[float, float, float, float] = None):
        """Request a redraw. If `immediate` is False, coalesce redraws and schedule one.

        `region` is optional canvas coords (vx0, vy0, vx1, vy1) for future local redraw support.
        """
        try:
            t0 = time.time()
            self._redraw_region = region
            self._needs_redraw = True
            if immediate:
                self._do_redraw()
                logging.debug(f"request_redraw immediate completed in {time.time()-t0:.4f}s")
                return
            if self._redraw_job is None:
                try:
                    self._redraw_job = self.view.after(50, self._do_redraw)
                except Exception:
                    # fallback: immediate
                    self._do_redraw()
            logging.debug(f"request_redraw scheduled (immediate={immediate}) took {time.time()-t0:.4f}s")
        except Exception:
            logging.exception('request_redraw failed')

    def _do_redraw(self):
        try:
            t0 = time.time()
            if self._redraw_job:
                try:
                    self.view.after_cancel(self._redraw_job)
                except Exception:
                    pass
            self._redraw_job = None
            self._needs_redraw = False
            region = self._redraw_region
            self._redraw_region = None
            try:
                self._draw_graph(batch=False, region=region)
            except TypeError:
                # older fallback
                self._draw_graph(batch=False)
            logging.debug(f"_do_redraw total {time.time()-t0:.3f}s")
        except Exception:
            logging.exception('_do_redraw failed')

    def _compute_layout(self):
        nodes = list(self.visible_nodes or self.model.nodes.keys())
        if not nodes:
            return

        # determine virtual world bounds from canvas scrollregion (so layout occurs in world coords)
        c = self.view.canvas
        try:
            sr = c.cget('scrollregion')
            if sr:
                if isinstance(sr, str):
                    parts = list(map(float, sr.split()))
                else:
                    parts = list(sr)
                left, top, right, bottom = parts
                w = max(1.0, right - left)
                h = max(1.0, bottom - top)
                world_cx = (left + right) / 2.0
                world_cy = (top + bottom) / 2.0
            else:
                w = c.winfo_width() or 800
                h = c.winfo_height() or 600
                world_cx = w / 2.0
                world_cy = h / 2.0
        except Exception:
            w = c.winfo_width() or 800
            h = c.winfo_height() or 600
            world_cx = w / 2.0
            world_cy = h / 2.0

        mode = (self.view.mode_var.get() or 'force') if hasattr(self.view, 'mode_var') else 'force'
        use_cluster_user = bool(getattr(self.view, 'clustering_var', tk.BooleanVar(value=False)).get())

        # determine cluster key from UI (topdir, subsystem, scanner, none)
        try:
            cluster_key = (self.view.cluster_key_var.get() or 'topdir')
        except Exception:
            cluster_key = 'topdir'

        # build clusters map
        clusters = {}
        for nid in nodes:
            key = 'root'
            try:
                if cluster_key == 'topdir' or cluster_key is None:
                    label = (self.model.nodes.get(nid).label or nid)
                    parts = label.split('/')
                    key = parts[1] if len(parts) > 1 and parts[0] == 'src' else parts[0]
                elif cluster_key == 'subsystem':
                    meta = self.model.nodes.get(nid).meta if nid in self.model.nodes else {}
                    key = meta.get('subsystem') or meta.get('module') or 'unknown'
                elif cluster_key == 'scanner':
                    gaps = self.model.gaps.get(nid) or []
                    key = gaps[0].get('scanner') if gaps else 'unknown'
                elif cluster_key == 'none':
                    key = nid
                else:
                    label = (self.model.nodes.get(nid).label or nid)
                    key = label.split('/')[0] if label else 'root'
            except Exception:
                key = 'root'
            clusters.setdefault(str(key) or 'root', []).append(nid)

        # store cluster map for LOD/aggregation
        self._cluster_map = clusters
        # update cluster count label if available
        try:
            if hasattr(self.view, 'cluster_count_var') and self.view.cluster_count_var is not None:
                self.view.cluster_count_var.set(f"Clusters: {len(clusters)}")
        except Exception:
            pass

        # compute superclusters if many clusters (group clusters into superclusters)
        self._superclusters = {}
        try:
            if len(clusters) > 30:
                # simple supercluster: group cluster keys by first char
                sc = {}
                for ck, members in clusters.items():
                    k = (ck or 'root')[0].upper() if ck else 'R'
                    sc.setdefault(k, []).extend(members)
                # store mapping from superkey->members
                self._superclusters = sc
        except Exception:
            self._superclusters = {}

        # update cluster listbox with keys and counts for user inspection
        try:
            if getattr(self.view, 'cluster_listbox', None):
                self.view.cluster_listbox.delete(0, tk.END)
                for ck, members in sorted(self._cluster_map.items()):
                    self.view.cluster_listbox.insert(tk.END, f"{ck} ({len(members)})")
        except Exception:
            pass

        if mode == 'galaxy' or use_cluster_user:
            # place cluster centers on a circle around the world center
            cx = world_cx
            cy = world_cy
            R = min(w, h) * 0.35
            k = len(clusters)
            if k == 0:
                return
            for i, (ck, members) in enumerate(sorted(clusters.items())):
                a = 2 * math.pi * i / k
                ccx = cx + R * math.cos(a)
                ccy = cy + R * math.sin(a)
                # arrange members around cluster center
                m = len(members)
                if m == 1:
                    self._target_positions[members[0]] = (ccx, ccy)
                else:
                    r2 = min(w, h) * (0.05 + 0.25 * (m / max(10, m)))
                    for j, nid in enumerate(members):
                        aa = 2 * math.pi * j / m
                        self._target_positions[nid] = (ccx + r2 * math.cos(aa), ccy + r2 * math.sin(aa))
        else:
            # prefer a local force-simulation that includes gravity to avoid drifting
            # into less-informative layouts; fallback to networkx if available
            # use_cluster_user indicates explicit user clustering toggle
            if mode == 'force' and not use_cluster_user:
                # run expensive force simulation off the main thread to avoid UI freeze
                try:
                    self.view.status.config(text="Computing force layout...")
                except Exception:
                    pass

                def layout_job():
                    # quick pass then refinement; return mapping
                    t0 = time.time()
                    try:
                        # determine iteration counts based on node count and user limit
                        user_max = 0
                        try:
                            user_max = int(getattr(self.view, 'layout_max_iter_var', tk.IntVar(value=40)).get())
                        except Exception:
                            user_max = 40
                        n_nodes = len(nodes)
                        # choose quick pass proportionally smaller for large graphs
                        quick_iters = max(8, min(30, int(15 * (2000.0 / max(2000.0, n_nodes)))))
                        # full refinement capped by user setting and graph size
                        refinement_iters = max(0, min(user_max, 2 * int(user_max * (2000.0 / max(2000.0, n_nodes)))))
                        logging.debug(f"layout_job: quick_iters={quick_iters}, refinement_iters={refinement_iters}, nodes={n_nodes}")
                        logging.debug(f"layout_job: starting quick pass ({quick_iters} iters)")
                        pos = self._force_simulate(nodes, self.model.edges, w, h, iterations=quick_iters)
                        logging.debug(f"layout_job: quick pass done in {time.time()-t0:.3f}s")

                        # refinement pass
                        try:
                            if refinement_iters > 0:
                                t1 = time.time()
                                logging.debug(f"layout_job: starting refinement ({refinement_iters} iters)")
                                pos2 = self._force_simulate(nodes, self.model.edges, w, h, iterations=refinement_iters)
                                logging.debug(f"layout_job: refinement done in {time.time()-t1:.3f}s")
                                pos.update(pos2)
                        except Exception:
                            logging.exception('layout refinement failed')

                        logging.debug(f"layout_job: total time {time.time()-t0:.3f}s")
                        return pos
                    except Exception:
                        logging.exception('layout_job failed')
                        return {}

                try:
                    # cancel previous layout if running
                    if self._layout_future and not self._layout_future.done():
                        try:
                            logging.debug('Cancelling previous layout future')
                            self._layout_future.cancel()
                        except Exception:
                            pass
                    self._layout_future = self._layout_executor.submit(layout_job)

                    def when_done(fut):
                        try:
                            pos = fut.result()
                            def apply_pos():
                                try:
                                    if not pos:
                                        try:
                                            self.view.status.config(text="Layout produced no positions")
                                        except Exception:
                                            pass
                                        return
                                    self._target_positions = pos
                                    # initialize start positions if missing
                                    for nid, tgt in self._target_positions.items():
                                        if nid not in self.node_positions:
                                            self.node_positions[nid] = tgt
                                    self._anim_frames = 0
                                    try:
                                        if self._batch_draw_job:
                                            self.view.after_cancel(self._batch_draw_job)
                                    except Exception:
                                        pass
                                    self._animate_to_target()
                                    try:
                                        self.view.status.config(text=f"Layout ready ({len(pos)} nodes)")
                                    except Exception:
                                        pass
                                except Exception:
                                    logging.exception('apply_pos failed')
                            try:
                                self.view.after(1, apply_pos)
                            except Exception:
                                apply_pos()
                        except Exception:
                            logging.exception('when_done processing failed')
                            try:
                                self.view.status.config(text="Layout failed, using fallback")
                            except Exception:
                                pass
                    self._layout_future.add_done_callback(when_done)
                    return
                except Exception:
                    logging.exception('Submitting layout_job failed, falling back to immediate compute')
                    pos = self._force_simulate(nodes, self.model.edges, w, h, iterations=30)
                    for nid, p in pos.items():
                        self._target_positions[nid] = p
            elif HAS_NX:
                G = nx.Graph()
                G.add_nodes_from(nodes)
                for e in self.model.edges:
                    if e.source in nodes and e.target in nodes:
                        G.add_edge(e.source, e.target)
                pos = nx.spring_layout(G, scale=min(w, h) * 0.4)
                # convert to screen coords
                cx = w / 2
                cy = h / 2
                for nid, p in pos.items():
                    self._target_positions[nid] = (cx + p[0], cy + p[1])
            else:
                # fallback: circle layout
                cx = w / 2
                cy = h / 2
                r = min(w, h) * 0.35
                n = len(nodes)
                for i, nid in enumerate(nodes):
                    a = 2 * math.pi * i / n
                    self._target_positions[nid] = (cx + r * math.cos(a), cy + r * math.sin(a))

    def _force_simulate(self, nodes: List[str], edges: List, w: int, h: int, iterations: int = 50):
        """Simple Fruchterman-Reingold force simulation with gravity toward center.

        Returns mapping nid -> (x,y) in canvas coordinates.
        """
        if not nodes:
            return {}
        t_start = time.time()
        # initialize positions (spread randomly) on a larger virtual area
        import random
        # determine virtual canvas/world bounds from canvas scrollregion so nodes aren't clamped to widget edges
        c = self.view.canvas
        try:
            sr = c.cget('scrollregion')
            if sr:
                if isinstance(sr, str):
                    parts = list(map(float, sr.split()))
                else:
                    parts = list(sr)
                left, top, right, bottom = parts
            else:
                left, top, right, bottom = -2000.0, -2000.0, 2000.0, 2000.0
        except Exception:
            left, top, right, bottom = -2000.0, -2000.0, 2000.0, 2000.0
        virt_w = max(1.0, right - left)
        virt_h = max(1.0, bottom - top)
        center_x = (left + right) / 2.0
        center_y = (top + bottom) / 2.0
        pos = {nid: (random.uniform(left, right), random.uniform(top, bottom)) for nid in nodes}
        area = float(virt_w * virt_h)
        N = max(1, len(nodes))
        k = math.sqrt(area / N)
        t = max(virt_w, virt_h) / 10.0
        dt = t / float(max(1, iterations))
        # build adjacency set for faster checks
        adj = {nid: set() for nid in nodes}
        for e in edges:
            if getattr(e, 'source', None) in nodes and getattr(e, 'target', None) in nodes:
                adj[e.source].add(e.target)
                adj[e.target].add(e.source)

        gravity_coeff = 0.05  # pull toward center
        eps = 1e-6
        # center already defined above for virtual area

        # Barnes-Hut quadtree implementation (2D)
        class QuadNode:
            __slots__ = ('x0', 'y0', 'x1', 'y1', 'cx', 'cy', 'mass', 'point', 'children')

            def __init__(self, x0, y0, x1, y1):
                self.x0 = x0; self.y0 = y0; self.x1 = x1; self.y1 = y1
                self.cx = 0.0; self.cy = 0.0; self.mass = 0.0
                self.point = None  # (nid, x, y)
                self.children = None

            def contains(self, x, y):
                return (self.x0 <= x <= self.x1) and (self.y0 <= y <= self.y1)

            def subdivide(self):
                mx = (self.x0 + self.x1) / 2.0
                my = (self.y0 + self.y1) / 2.0
                self.children = [
                    QuadNode(self.x0, self.y0, mx, my),  # nw
                    QuadNode(mx, self.y0, self.x1, my),  # ne
                    QuadNode(self.x0, my, mx, self.y1),  # sw
                    QuadNode(mx, my, self.x1, self.y1),  # se
                ]

            def insert(self, nid, x, y):
                # insert point into this node
                if not self.contains(x, y):
                    return False
                # if empty leaf, store point
                if self.point is None and self.children is None and self.mass == 0.0:
                    self.point = (nid, x, y)
                    self.mass = 1.0
                    self.cx = x
                    self.cy = y
                    return True
                # if this is a leaf but already has a point, try to subdivide
                if self.children is None:
                    width = max(self.x1 - self.x0, 0.0)
                    height = max(self.y1 - self.y0, 0.0)
                    # avoid subdividing beyond float precision / zero-size cells
                    if width <= 1e-6 or height <= 1e-6:
                        # collapse into a small bucket: store multiple points in this node
                        if isinstance(self.point, list):
                            self.point.append((nid, x, y))
                        else:
                            # convert single point to list of points
                            if self.point is not None:
                                self.point = [self.point, (nid, x, y)]
                            else:
                                self.point = (nid, x, y)
                        # update mass and center of mass
                        pts = self.point if isinstance(self.point, list) else [self.point]
                        m = float(len(pts))
                        sx = sum(p[1] for p in pts)
                        sy = sum(p[2] for p in pts)
                        self.mass = m
                        self.cx = sx / m
                        self.cy = sy / m
                        return True
                    # otherwise subdivide normally
                    self.subdivide()
                    if self.point is not None:
                        # move existing point(s) into children
                        existing = self.point
                        self.point = None
                        pts = existing if isinstance(existing, list) else [existing]
                        for pnid, px, py in pts:
                            for ch in self.children:
                                if ch.insert(pnid, px, py):
                                    break
                # insert into child
                for ch in self.children:
                    if ch.insert(nid, x, y):
                        break
                # update center of mass and mass
                self.mass = 0.0
                sx = 0.0; sy = 0.0
                for ch in self.children:
                    if ch.mass > 0:
                        self.mass += ch.mass
                        sx += ch.cx * ch.mass
                        sy += ch.cy * ch.mass
                if self.mass > 0:
                    self.cx = sx / self.mass
                    self.cy = sy / self.mass
                return True

        def build_quadtree(pos_dict):
            # quadtree over the virtual area so nodes outside visible widget are included
            root = QuadNode(left, top, right, bottom)
            for nid, (x, y) in pos_dict.items():
                root.insert(nid, x, y)
            return root

        def apply_repulsion_from_quad(node: QuadNode, ux, uy, theta, k_val, exclude_nid, disp_u):
            # traverse quadtree and accumulate repulsive force on (ux,uy)
            stack = [node]
            while stack:
                n = stack.pop()
                if n.mass == 0:
                    continue
                dx = ux - n.cx
                dy = uy - n.cy
                dist = math.hypot(dx, dy) + eps
                width = max(n.x1 - n.x0, 1e-6)
                if (n.children is None) or (width / dist < theta):
                    # approximate as single mass
                    # mass-weighted repulsion: M * (k^2) / dist
                    force = (n.mass * (k_val * k_val)) / dist
                    fx = (dx / dist) * force
                    fy = (dy / dist) * force
                    disp_u[0] += fx
                    disp_u[1] += fy
                else:
                    # need to open node
                    if n.point is not None:
                        if isinstance(n.point, list):
                            for pnid, px, py in n.point:
                                if pnid != exclude_nid:
                                    ddx = ux - px
                                    ddy = uy - py
                                    dd = math.hypot(ddx, ddy) + eps
                                    f = (k_val * k_val) / dd
                                    disp_u[0] += (ddx / dd) * f
                                    disp_u[1] += (ddy / dd) * f
                        else:
                            pnid, px, py = n.point
                            if pnid != exclude_nid:
                                ddx = ux - px
                                ddy = uy - py
                                dd = math.hypot(ddx, ddy) + eps
                                f = (k_val * k_val) / dd
                                disp_u[0] += (ddx / dd) * f
                                disp_u[1] += (ddy / dd) * f
                    if n.children is not None:
                        for ch in n.children:
                            if ch.mass > 0:
                                stack.append(ch)

        theta = 0.5

        for it in range(iterations):
            it_start = time.time()
            disp = {nid: [0.0, 0.0] for nid in nodes}

            # Decide quadtree rebuild frequency for performance on large graphs
            if N > 2000:
                rebuild_every = 3
            elif N > 800:
                rebuild_every = 2
            else:
                rebuild_every = 1

            # build quadtree from current positions (maybe reuse between iterations)
            if it == 0 or (it % rebuild_every) == 0:
                qt_build_start = time.time()
                qt_root = build_quadtree(pos)
                prev_qt_root = qt_root
                logging.debug(f"_force_simulate: quadtree build {time.time()-qt_build_start:.3f}s (rebuild_every={rebuild_every})")
            else:
                # reuse previously built quadtree for a few iterations
                qt_root = prev_qt_root

            # repulsive forces via Barnes-Hut
            for u in nodes:
                ux, uy = pos[u]
                apply_repulsion_from_quad(qt_root, ux, uy, theta, k, u, disp[u])

            # attractive forces along edges (exact)
            for u in nodes:
                for v in adj.get(u, []):
                    ux, uy = pos[u]
                    vx, vy = pos[v]
                    dx = ux - vx
                    dy = uy - vy
                    dist = math.hypot(dx, dy) + eps
                    # spring force
                    force = (dist * dist) / k
                    fx = (dx / dist) * force
                    fy = (dy / dist) * force
                    disp[u][0] -= fx
                    disp[u][1] -= fy

            # gravity toward center
            for u in nodes:
                ux, uy = pos[u]
                dx = ux - center_x
                dy = uy - center_y
                gx = -gravity_coeff * dx
                gy = -gravity_coeff * dy
                disp[u][0] += gx
                disp[u][1] += gy

            # limit maximum displacement and apply
            for u in nodes:
                dx, dy = disp[u]
                disp_len = math.hypot(dx, dy)
                if disp_len > 0:
                    limited = min(disp_len, t)
                    nx = pos[u][0] + (dx / disp_len) * limited
                    ny = pos[u][1] + (dy / disp_len) * limited
                else:
                    nx, ny = pos[u]
                # keep within virtual bounds (avoid forcing to widget edges)
                nx = min(right - 10, max(left + 10, nx))
                ny = min(bottom - 10, max(top + 10, ny))
                pos[u] = (nx, ny)

            # cool temperature
            t = max(0.1, t - dt)
            logging.debug(f"_force_simulate: iter {it+1}/{iterations} took {time.time()-it_start:.3f}s")

        total = time.time() - t_start
        logging.debug(f"_force_simulate total time {total:.3f}s for {len(nodes)} nodes (barnes-hut, theta={theta})")
        return pos

        # prepare animation from current positions to target
        for nid, tgt in self._target_positions.items():
            if nid not in self.node_positions:
                # initialize missing start positions at target to avoid jump
                self.node_positions[nid] = tgt
        # start tween animation
        self._anim_frames = 0
        try:
            self.view.after_cancel(self._batch_draw_job)
        except Exception:
            pass
        self._animate_to_target()

    def _draw_graph(self, batch=False, region=None):
        t0 = time.time()
        c = self.view.canvas
        # respect user clustering toggle when drawing
        use_cluster_user = bool(getattr(self.view, 'clustering_var', tk.BooleanVar(value=False)).get())
        if not batch:
            # remove only dynamic edge items; keep node items and reuse them
            try:
                c.delete("edge")
            except Exception:
                pass
            # clear previous cluster aggregates (we recreate them)
            try:
                for vals in list(self._cluster_canvas_items.values()):
                    for iid in vals:
                        try:
                            if iid:
                                c.delete(iid)
                        except Exception:
                            pass
                self._cluster_canvas_items.clear()
            except Exception:
                pass
            # draw background grid and rulers if enabled
            try:
                if getattr(self.view, 'grid_var', tk.BooleanVar(value=False)).get():
                    self._draw_background(c)
            except Exception:
                pass

        # determine nodes to draw (respect visible_nodes if set)
        nodes = list(self.visible_nodes or self.node_positions.keys())

        # heatmap option: color intensity by gap count
        heatmap = bool(getattr(self.view, 'heatmap_var', tk.BooleanVar(value=False)).get())
        # compute max gap count for visible nodes
        max_gaps = 0
        gap_counts = {}
        for nid in nodes:
            cnt = len(self.model.gaps.get(nid) or self.model.gaps.get(self.model.nodes.get(nid).label if nid in self.model.nodes else '') or [])
            gap_counts[nid] = cnt
            if cnt > max_gaps:
                max_gaps = cnt

        def heat_color(count):
            if max_gaps <= 0:
                return '#2b7'
            s = float(count) / float(max_gaps)
            r = int(55 + 200 * s)
            g = int(55 + 200 * (1.0 - s))
            b = 80
            return f"#{r:02x}{g:02x}{b:02x}"

        # determine viewport bounds for culling
        try:
            if region:
                vx0, vy0, vx1, vy1 = region
            else:
                vx0 = c.canvasx(0)
                vy0 = c.canvasy(0)
                vx1 = c.canvasx(c.winfo_width() or 800)
                vy1 = c.canvasy(c.winfo_height() or 600)
        except Exception:
            vx0, vy0, vx1, vy1 = -1e9, -1e9, 1e9, 1e9
        margin = 120

        # compute which nodes are in (or near) viewport
        nodes_in_view = []
        for nid in nodes:
            x, y = self.node_positions.get(nid, (0, 0))
            if x >= vx0 - margin and x <= vx1 + margin and y >= vy0 - margin and y <= vy1 + margin:
                nodes_in_view.append(nid)

        # choose nodes to draw (if nothing visible, fallback to all nodes)
        nodes_to_draw = nodes_in_view if nodes_in_view else nodes

        # adaptive batch size based on visible nodes
        total_visible = len(nodes_to_draw)
        if total_visible > 3000:
            self._batch_size = 120
        elif total_visible > 1500:
            self._batch_size = 250
        elif total_visible > 800:
            self._batch_size = 400
        else:
            self._batch_size = 800

        # draw edges only on full redraw and only if both endpoints are in the viewport set
        e_start = time.time()
        if not batch:
            # compute node radius (scaled by UI control)
            try:
                scale_all = float(getattr(self.view, 'node_size_var', tk.DoubleVar(value=1.0)).get())
            except Exception:
                scale_all = 1.0
            base_r = self.NODE_RADIUS
            r_global = max(4, base_r * scale_all)
            # reuse edge canvas items and cull to visible edges
            desired_edges = set()
            for e in self.model.edges:
                s = e.source
                t = e.target
                if s in nodes_in_view and t in nodes_in_view and s in self.node_positions and t in self.node_positions:
                    x1, y1 = self.node_positions[s]
                    x2, y2 = self.node_positions[t]
                    dx = x2 - x1
                    dy = y2 - y1
                    dist = math.hypot(dx, dy)
                    if dist <= 1e-6:
                        continue
                    ux = dx / dist
                    uy = dy / dist
                    # offset line endpoints to node perimeters to avoid overlap
                    sx = x1 + ux * r_global
                    sy = y1 + uy * r_global
                    tx = x2 - ux * r_global
                    ty = y2 - uy * r_global
                    key = (s, t)
                    desired_edges.add(key)
                    if key in self._edge_canvas_items:
                        lid = self._edge_canvas_items[key]
                        try:
                            c.coords(lid, sx, sy, tx, ty)
                            c.itemconfig(lid, fill="#888")
                        except Exception:
                            pass
                    else:
                        try:
                            lid = c.create_line(sx, sy, tx, ty, fill="#888", width=1.0, arrow='last', arrowshape=(8,10,4), tags=("edge",))
                        except Exception:
                            lid = c.create_line(sx, sy, tx, ty, fill="#888", tags=("edge",))
                        self._edge_canvas_items[key] = lid
            # cleanup edges that are no longer desired
            try:
                existing_edges = set(self._edge_canvas_items.keys())
                for key in existing_edges - desired_edges:
                    lid = self._edge_canvas_items.pop(key, None)
                    try:
                        if lid:
                            c.delete(lid)
                    except Exception:
                        pass
            except Exception:
                pass
        e_time = time.time() - e_start
        # decide on symbolic LOD (aggregate clusters when zoomed out)
        # compute overall bbox of positions
        minx = min((self.node_positions.get(nid, (0, 0))[0] for nid in nodes), default=0)
        maxx = max((self.node_positions.get(nid, (0, 0))[0] for nid in nodes), default=0)
        bbox_w = maxx - minx
        view_w = max(1.0, (vx1 - vx0))
        zoom_factor = view_w / (bbox_w + 1.0)

        display_items: List[Tuple[str, Any]] = []
        # choose LOD level: supercluster -> cluster -> node
        use_super = zoom_factor < self.SUPERCLUSTER_ZOOM and bool(self._superclusters)
        # respect explicit user toggle OR automatic zoom-based clustering
        use_cluster = bool(use_cluster_user) or (zoom_factor < self.CLUSTER_ZOOM and bool(self._cluster_map))

        # debug: log cluster keys and counts for diagnosis
        try:
            ck_debug = {k: len(v) for k, v in list(self._cluster_map.items())[:50]}
            logging.debug(f"Cluster debug: keys_sample={ck_debug} total_clusters={len(self._cluster_map)} use_cluster_user={use_cluster_user} zoom_factor={zoom_factor:.3f}")
        except Exception:
            pass

        def centroid_of(members):
            xs = [self.node_positions.get(n, (0, 0))[0] for n in members]
            ys = [self.node_positions.get(n, (0, 0))[1] for n in members]
            if not xs:
                return None
            return (sum(xs) / len(xs), sum(ys) / len(ys))

        if use_super:
            # show superclusters (groupings of cluster keys)
            for sk, members in sorted(self._superclusters.items()):
                cent = centroid_of(members)
                if not cent:
                    continue
                cx, cy = cent
                if cx >= vx0 - margin and cx <= vx1 + margin and cy >= vy0 - margin and cy <= vy1 + margin:
                    display_items.append(('super', (sk, cx, cy, len(members))))
        elif use_cluster:
            # show clusters as aggregated nodes
            for ck, members in sorted(self._cluster_map.items()):
                cent = centroid_of(members)
                if not cent:
                    continue
                cx, cy = cent
                if cx >= vx0 - margin and cx <= vx1 + margin and cy >= vy0 - margin and cy <= vy1 + margin:
                    display_items.append(('cluster', (ck, cx, cy, len(members))))
                else:
                    # small clusters can still show members
                    if len(members) < 8:
                        for n in members:
                            display_items.append(('node', n))
        else:
            # default: show nodes, but compress very large clusters into single aggregates
            # find clusters larger than threshold
            large_clusters = {ck: members for ck, members in self._cluster_map.items() if len(members) >= self.LARGE_CLUSTER_SIZE}
            large_nodes_shown = set()
            for nid in nodes_to_draw:
                # if nid belongs to a large cluster and zoom not yet expanded, skip individual nodes
                skip = False
                for ck, mems in large_clusters.items():
                    if nid in mems:
                        skip = True
                        break
                if skip:
                    continue
                display_items.append(('node', nid))
            # add aggregated visuals for large clusters so they appear as single elements
            for ck, members in large_clusters.items():
                cent = centroid_of(members)
                if not cent:
                    continue
                cx, cy = cent
                if cx >= vx0 - margin and cx <= vx1 + margin and cy >= vy0 - margin and cy <= vy1 + margin:
                    display_items.append(('cluster', (ck, cx, cy, len(members))))

        total = len(display_items)
        if total == 0:
            # fallback for diagnostics: if clustering/L0D removed all items, show first nodes
            logging.debug("_draw_graph: no display_items selected — falling back to showing first nodes for debug")
            fallback = []
            sample = list(nodes)[:min(100, len(nodes))]
            for n in sample:
                fallback.append(('node', n))
            display_items = fallback
            total = len(display_items)

        start = getattr(self, '_batch_start', 0)
        end = min(start + self._batch_size, total)

        n_start = time.time()
        # reuse/create/remove node canvas items to avoid expensive create/delete
        desired_nodes = [it[1] for it in display_items[start:end] if it[0] == 'node']
        desired_node_set = set(desired_nodes)
        # process node items: update existing or create new
        LABEL_THRESHOLD = 1000
        try:
            scale = float(getattr(self.view, 'node_size_var', tk.DoubleVar(value=1.0)).get())
        except Exception:
            scale = 1.0
        base_r = self.NODE_RADIUS
        for item in display_items[start:end]:
            if item[0] == 'node':
                nid = item[1]
                x, y = self.node_positions.get(nid, (0, 0))
                r = int(base_r * scale)
                gaps = self.model.gaps.get(nid) or self.model.gaps.get(self.model.nodes.get(nid).label if nid in self.model.nodes else "") or []
                if heatmap:
                    fill = heat_color(gap_counts.get(nid, 0))
                else:
                    fill = "#e88" if gaps else "#2b7"
                tag = f"node_{nid}"
                if nid in self._node_canvas_items:
                    oval_id, text_id = self._node_canvas_items[nid]
                    try:
                        c.coords(oval_id, x - r, y - r, x + r, y + r)
                        c.itemconfig(oval_id, fill=fill)
                    except Exception:
                        pass
                    if total_visible <= LABEL_THRESHOLD:
                        label = os.path.basename(self.model.nodes.get(nid).label if nid in self.model.nodes else str(nid))
                        if text_id:
                            try:
                                c.coords(text_id, x, y)
                                c.itemconfig(text_id, text=label)
                            except Exception:
                                pass
                        else:
                            try:
                                tid = c.create_text(x, y, text=label, font=("Arial", 9), tags=(tag,))
                                self._node_canvas_items[nid] = (oval_id, tid)
                            except Exception:
                                pass
                    else:
                        # remove text if present to save draw time
                        if text_id:
                            try:
                                c.delete(text_id)
                            except Exception:
                                pass
                            self._node_canvas_items[nid] = (oval_id, None)
                else:
                    try:
                        oval = c.create_oval(x - r, y - r, x + r, y + r, fill=fill, outline="#070", tags=(tag,))
                    except Exception:
                        oval = None
                    text_obj = None
                    if total_visible <= LABEL_THRESHOLD:
                        try:
                            text_obj = c.create_text(x, y, text=os.path.basename(self.model.nodes.get(nid).label if nid in self.model.nodes else str(nid)), font=("Arial", 9), tags=(tag,))
                        except Exception:
                            text_obj = None
                    self._node_canvas_items[nid] = (oval, text_obj)
                    # bind once
                    try:
                        if oval:
                            c.tag_bind(tag, '<Enter>', lambda e, nid=nid: self._on_node_hover(e, nid))
                            c.tag_bind(tag, '<Leave>', lambda e, nid=nid: self._on_node_leave(e, nid))
                            c.tag_bind(tag, '<Button-1>', lambda e, nid=nid: self._on_node_click(e, nid))
                    except Exception:
                        pass
            else:
                # cluster aggregate: create fresh (clusters are fewer)
                ck, cx, cy, count = item[1]
                r = max(10, min(80, int(math.sqrt(count) * 6)))
                tag = f"agg_cluster_{ck}"
                try:
                    oval = c.create_oval(cx - r, cy - r, cx + r, cy + r, fill="#888", outline="#333", tags=(tag,))
                    text_id = c.create_text(cx, cy, text=f"{ck} ({count})", font=("Arial", 10, "bold"), tags=(tag,))
                except Exception:
                    oval = None
                    text_id = None
                self._cluster_canvas_items[ck] = (oval, text_id)
                try:
                    c.tag_bind(tag, '<Button-1>', lambda e, ck=ck: self._on_cluster_click(e, ck))
                except Exception:
                    pass
                # tooltip handlers
                def make_agg_tooltip(evt, ck=ck, members=members):
                    if not members:
                        return
                    txt = f"Cluster: {ck}\nCount: {len(members)}\nTop members:\n"
                    top = members[:10]
                    for m in top:
                        lbl = self.model.nodes.get(m).label if m in self.model.nodes else str(m)
                        gcount = len(self.model.gaps.get(m) or [])
                        txt += f" - {lbl} ({gcount} gaps)\n"
                    type_counts = {}
                    for m in members:
                        for g in (self.model.gaps.get(m) or []):
                            gt = g.get('type') or g.get('gap_type') or 'unknown'
                            type_counts[gt] = type_counts.get(gt, 0) + 1
                    if type_counts:
                        txt += "\nGap types:\n"
                        for gt, cnt in sorted(type_counts.items(), key=lambda t: -t[1])[:5]:
                            txt += f" - {gt}: {cnt}\n"
                    t = tk.Toplevel(self.view)
                    t.wm_overrideredirect(True)
                    lblw = ttk.Label(t, text=txt, relief=tk.SOLID, padding=4)
                    lblw.pack()
                    try:
                        t.wm_geometry(f"+{evt.x_root+12}+{evt.y_root+12}")
                    except Exception:
                        pass
                    self._agg_tip = t
                def destroy_agg_tooltip(evt):
                    try:
                        if getattr(self, '_agg_tip', None):
                            self._agg_tip.destroy()
                    except Exception:
                        pass
                try:
                    c.tag_bind(tag, '<Enter>', make_agg_tooltip)
                    c.tag_bind(tag, '<Leave>', destroy_agg_tooltip)
                except Exception:
                    pass

        # remove node canvas items that are no longer desired
        try:
            existing = set(self._node_canvas_items.keys())
            to_remove = existing - desired_node_set
            for nid in to_remove:
                oval_id, text_id = self._node_canvas_items.pop(nid, (None, None))
                try:
                    if oval_id:
                        c.delete(oval_id)
                except Exception:
                    pass
                try:
                    if text_id:
                        c.delete(text_id)
                except Exception:
                    pass
        except Exception:
            pass
            # NOTE: node drawing handled above per-item; no duplicate drawing here

        n_time = time.time() - n_start
        total_time = time.time() - t0
        logging.debug(f"_draw_graph edges_time={e_time:.3f}s nodes_time={n_time:.3f}s total_time={total_time:.3f}s items={total}")

        # update scrollregion to include all drawn content (make content effectively infinite)
        try:
            # determine bbox from node_positions
            all_x = [p[0] for p in self.node_positions.values()] if self.node_positions else []
            all_y = [p[1] for p in self.node_positions.values()] if self.node_positions else []
            if all_x and all_y:
                minx = min(all_x) - 200
                maxx = max(all_x) + 200
                miny = min(all_y) - 200
                maxy = max(all_y) + 200
            else:
                minx, miny, maxx, maxy = -10000, -10000, 10000, 10000
            try:
                c.configure(scrollregion=(minx, miny, maxx, maxy))
            except Exception:
                pass
        except Exception:
            logging.exception('Failed to update scrollregion')

        if end < total:
            self._batch_start = end
            # update progress UI
            try:
                if getattr(self.view, 'progress', None):
                    try:
                        self.view.progress.config(mode='determinate', maximum=total)
                        self.view.progress['value'] = end
                        # ensure visible
                        try:
                            self.view.progress.pack(side=tk.RIGHT, padx=(6,8))
                        except Exception:
                            pass
                    except Exception:
                        pass
                try:
                    self.view.status.config(text=f"Drawing: {end}/{total}")
                except Exception:
                    pass
            except Exception:
                pass
            try:
                self._batch_draw_job = self.view.after(20, lambda: self._draw_graph(batch=True))
            except Exception:
                pass
        else:
            self._batch_start = 0
            self._batch_draw_job = None
            # finalize progress UI
            try:
                if getattr(self.view, 'progress', None):
                    try:
                        self.view.progress['value'] = total
                        # hide after short delay
                        self.view.after(120, lambda: getattr(self.view, 'progress').pack_forget())
                    except Exception:
                        pass
                try:
                    self.view.status.config(text=f"Ready ({total} items)")
                except Exception:
                    pass
            except Exception:
                pass

    def on_canvas_resize(self, event):
        self._compute_layout()
        self.request_redraw()

    def _draw_background(self, c: tk.Canvas):
        """Draw grid lines centered on canvas center and simple rulers on top/left."""
        try:
            # draw grid in world coordinates using canvas scrollregion/view
            vx0 = c.canvasx(0)
            vy0 = c.canvasy(0)
            vx1 = c.canvasx(c.winfo_width() or 800)
            vy1 = c.canvasy(c.winfo_height() or 600)
            spacing = int(getattr(self.view, 'grid_spacing_var', tk.IntVar(value=50)).get())
            color = '#e6e6e6'
            # vertical grid lines at multiples of spacing in world coords
            start_x = math.floor(vx0 / spacing) * spacing
            x = start_x
            while x <= vx1:
                c.create_line(x, vy0, x, vy1, fill=color, width=1)
                x += spacing
            # horizontal grid lines
            start_y = math.floor(vy0 / spacing) * spacing
            y = start_y
            while y <= vy1:
                c.create_line(vx0, y, vx1, y, fill=color, width=1)
                y += spacing

            # center axes (world origin) if visible
            try:
                if vx0 <= 0 <= vx1:
                    c.create_line(0, vy0, 0, vy1, fill='#999999', width=2)
                if vy0 <= 0 <= vy1:
                    c.create_line(vx0, 0, vx1, 0, fill='#999999', width=2)
            except Exception:
                pass

            # rulers: ticks and labels at top/left using world coords
            tick_color = '#333333'
            font = ("Arial", 8)
            # top ticks and labels along visible width
            x = start_x
            while x <= vx1:
                c.create_line(x, vy0, x, vy0 + 8, fill=tick_color)
                c.create_text(x, vy0 + 12, text=str(int(x)), font=font, fill=tick_color)
                x += spacing
            # left ticks and labels along visible height
            y = start_y
            while y <= vy1:
                c.create_line(vx0, y, vx0 + 8, y, fill=tick_color)
                c.create_text(vx0 + 18, y, text=str(int(y)), font=font, anchor='w', fill=tick_color)
                y += spacing
        except Exception:
            logging.exception('Error drawing background')

    def open_file_dialog(self):
        from tkinter import filedialog
        p = filedialog.askopenfilename(title="Open graph JSON", filetypes=[("JSON files","*.json"), ("All files","*")])
        if p:
            try:
                # reload model
                self.model = GraphModel(p)
                # update raw view
                try:
                    with open(p, 'r', encoding='utf-8') as f:
                        txt = f.read()
                except Exception:
                    txt = ''
                self.view.raw_text.delete('1.0', tk.END)
                self.view.raw_text.insert(tk.END, txt)
                # reapply filters
                self.apply_filters()
                self.view.status.config(text=f"Loaded: {p}")
            except Exception as e:
                messagebox.showerror("Load failed", str(e))

    # --- Canvas interaction handlers
    def _on_node_hover(self, event, nid: str):
        # show tooltip near cursor
        x = event.x_root + 16
        y = event.y_root + 16
        meta = self.model.nodes.get(nid).meta if nid in self.model.nodes else {}
        gaps = self.model.gaps.get(nid) or []
        # compute logical coords relative to center
        try:
            c = self.view.canvas
            # compute world center from scrollregion so tooltip shows logical world coords
            sr = c.cget('scrollregion')
            if sr:
                if isinstance(sr, str):
                    parts = list(map(float, sr.split()))
                else:
                    parts = list(sr)
                left, top, right, bottom = parts
                cx = (left + right) / 2.0
                cy = (top + bottom) / 2.0
            else:
                w = c.winfo_width() or 1
                h = c.winfo_height() or 1
                cx = w / 2
                cy = h / 2
            nx, ny = self.node_positions.get(nid, (0, 0))
            lx = nx - cx
            ly = ny - cy
            coord_txt = f"\nCoord: ({int(lx)},{int(ly)})"
        except Exception:
            coord_txt = ''
        txt = f"{self.model.nodes.get(nid).label if nid in self.model.nodes else nid}\nGaps: {len(gaps)}" + coord_txt
        # include brief meta if exists
        if meta:
            short = meta.get('summary') or meta.get('label') or ''
            if short:
                txt += f"\n{short}"

        if self._tooltip:
            try:
                self._tooltip.destroy()
            except Exception:
                pass
        self._tooltip = tk.Toplevel(self.view)
        self._tooltip.wm_overrideredirect(True)
        lbl = ttk.Label(self._tooltip, text=txt, relief=tk.SOLID, padding=4)
        lbl.pack()
        self._tooltip.wm_geometry(f"+{x}+{y}")

    def _on_node_leave(self, event, nid: str):
        if self._tooltip:
            try:
                self._tooltip.destroy()
            except Exception:
                pass
            self._tooltip = None

    def _on_node_click(self, event, nid: str):
        # center view on node
        if nid not in self.node_positions:
            return
        x, y = self.node_positions[nid]
        c = self.view.canvas
        try:
            # compute current viewport center in world coords
            vx0 = c.canvasx(0)
            vy0 = c.canvasy(0)
            vx1 = c.canvasx(c.winfo_width() or 1)
            vy1 = c.canvasy(c.winfo_height() or 1)
            vcx = (vx0 + vx1) / 2.0
            vcy = (vy0 + vy1) / 2.0
            dx = x - vcx
            dy = y - vcy
            # translate logical positions so node becomes centered
            self._translate_positions(-dx, -dy)
        except Exception:
            try:
                w = c.winfo_width() or 1
                h = c.winfo_height() or 1
                dx = x - (w / 2)
                dy = y - (h / 2)
                self._translate_positions(-dx, -dy)
            except Exception:
                pass
        # optional: slightly zoom in
        self._scale_canvas(1.05)

    def zoom_in(self):
        self._scale_canvas(1.15)

    def zoom_out(self):
        self._scale_canvas(1 / 1.15)

    def _scale_canvas(self, factor: float, center: Tuple[float, float]=None):
        # scale logical node positions around a center point, then redraw
        c = self.view.canvas
        if isinstance(factor, tuple):
            # legacy usage
            factor = float(factor)
        center = None
        # accept optional center arg passed via keyword in call
        try:
            # caller may pass center via attribute
            center = getattr(self, '_scale_center', None)
        except Exception:
            center = None
        if center is None:
            try:
                w = c.winfo_width() or 1
                h = c.winfo_height() or 1
                cx = w / 2
                cy = h / 2
            except Exception:
                cx, cy = 0, 0
        else:
            cx, cy = center

        # transform each logical node position
        for nid, (x, y) in list(self.node_positions.items()):
            nx = cx + (x - cx) * factor
            ny = cy + (y - cy) * factor
            self.node_positions[nid] = (nx, ny)

        # redraw (coalesced)
        try:
            self.request_redraw()
        except Exception:
            pass

    def _animate_to_target(self):
        # tween node_positions towards _target_positions over _anim_total_frames
        if not self._target_positions:
            return
        steps = max(1, self._anim_total_frames)
        t = float(self._anim_frames) / float(steps)
        # clamp t between 0..1
        t = max(0.0, min(1.0, t))
        changed = False
        for nid, tgt in self._target_positions.items():
            sx, sy = self.node_positions.get(nid, tgt)
            tx, ty = tgt
            nx = sx + (tx - sx) * t
            ny = sy + (ty - sy) * t
            if abs(nx - sx) > 0.1 or abs(ny - sy) > 0.1:
                changed = True
            self.node_positions[nid] = (nx, ny)

        # redraw in batches to keep UI responsive (coalesced)
        try:
            self.request_redraw()
        except Exception:
            pass

        if self._anim_frames < steps:
            self._anim_frames += 1
            try:
                self.view.after(30, self._animate_to_target)
            except Exception:
                pass

    def _update_legend_preview(self):
        try:
            if not getattr(self, 'legend_canvas', None):
                return
            c = self.legend_canvas
            c.delete('all')
            cmap = getattr(self.view, 'color_map_var', tk.StringVar(value='rg')).get()
            w = 120
            for i in range(w):
                s = i / float(w - 1)
                if cmap == 'rg':
                    r = int(55 + 200 * s)
                    g = int(55 + 200 * (1.0 - s))
                    b = 80
                elif cmap == 'br':
                    r = int(80 + 175 * (1.0 - s))
                    g = int(55 + 100 * s)
                    b = int(200 * s)
                else:
                    # simple viridis-like
                    r = int(68 + 180 * (1.0 - s))
                    g = int(1 + 220 * s)
                    b = int(84 + 100 * s)
                color = f"#{r:02x}{g:02x}{b:02x}"
                c.create_line(i, 0, i, 16, fill=color)
        except Exception:
            pass

    def _on_canvas_mousewheel(self, event):
        # support Windows and X11 events
        try:
            if hasattr(event, 'delta'):
                delta = event.delta
                # Windows: delta is multiple of 120
                if delta > 0:
                    factor = 1.1
                else:
                    factor = 1 / 1.1
            else:
                # X11: Button-4 (up) / Button-5 (down)
                if str(event.num) == '4':
                    factor = 1.1
                else:
                    factor = 1 / 1.1
        except Exception:
            factor = 1.1

        # get cursor position relative to canvas
        c = self.view.canvas
        try:
            cx = c.canvasx(event.x)
            cy = c.canvasy(event.y)
        except Exception:
            cx = cy = None
            if center is not None:
                try:
                    cx, cy = center
                except Exception:
                    cx = cy = None
            if cx is None or cy is None:
                try:
                    w = c.winfo_width() or 1
                    h = c.winfo_height() or 1
                    cx = w / 2
                    cy = h / 2
                except Exception:
                    cx, cy = 0, 0
    def _do_pan(self, event):
        if not self._pan_data:
            return
        px, py = self._pan_data
        dx = event.x - px
        dy = event.y - py
        # translate logical positions by dx,dy
        self._translate_positions(dx, dy)
        self._pan_data = (event.x, event.y)

    def _end_pan(self, event):
        self._pan_data = None

    def _translate_positions(self, dx: float, dy: float):
        try:
            for nid, (x, y) in list(self.node_positions.items()):
                self.node_positions[nid] = (x + dx, y + dy)
            # trigger a batched redraw (fast)
            try:
                if self._batch_draw_job:
                    self.view.after_cancel(self._batch_draw_job)
            except Exception:
                pass
            self._batch_start = 0
            self.request_redraw()
        except Exception:
            pass

    def _on_cluster_click(self, event, cluster_key: str):
        # expand cluster: set visible_nodes to cluster members and refocus
        members = self._cluster_map.get(cluster_key) or []
        if not members:
            return
        self.visible_nodes = members
        try:
            self._compute_layout()
            self.request_redraw()
            self.view.status.config(text=f"Expanded cluster {cluster_key} ({len(members)} nodes)")
        except Exception:
            pass

    def _on_cluster_list_select(self, event):
        try:
            lb = event.widget
            sel = lb.curselection()
            if not sel:
                return
            idx = sel[0]
            txt = lb.get(idx)
            # txt format: 'key (N)'
            key = txt.split(' (', 1)[0]
            # set cluster key in UI and enable clustering
            try:
                if hasattr(self.view, 'cluster_key_var'):
                    self.view.cluster_key_var.set(key)
                if hasattr(self.view, 'clustering_var'):
                    self.view.clustering_var.set(True)
                self._compute_layout()
                self.request_redraw()
                self.view.status.config(text=f"Clustering by: {key}")
            except Exception:
                pass
        except Exception:
            pass

    def fit_view(self):
        # recompute layout and redraw (fit)
        self._compute_layout()
        self.request_redraw(immediate=False)

    def toggle_networkx(self):
        global HAS_NX
        if not HAS_NX:
            messagebox.showinfo("NetworkX", "networkx not installed or available")
            return
        # flip behavior
        HAS_NX = not HAS_NX
        self.view.status.config(text=f"networkx layout: {HAS_NX}")
        self.fit_view()

    def show_about(self):
        messagebox.showinfo("About", "ThemisDB Graph Visualizer\nTkinter-based PoC")

    def on_node_select(self, event):
        # handle Treeview selection: get selected item id (we insert iid=nid)
        tv = event.widget
        sel = tv.selection()
        if not sel:
            return
        nid = sel[0]
        node = self.model.nodes.get(nid)
        label = node.label if node else nid
        self.view.status.config(text=f"Selected: {label}")
        # update details
        chunks = self.model.chunks.get(nid) or self.model.chunks.get(self.model.nodes.get(nid).label if nid in self.model.nodes else '') or []
        gaps = self.model.gaps.get(nid) or self.model.gaps.get(self.model.nodes.get(nid).label if nid in self.model.nodes else '') or []

        # populate chunks tree
        try:
            for ch in self.view.chunks_tree.get_children():
                self.view.chunks_tree.delete(ch)
            if chunks:
                for i, c in enumerate(chunks):
                    start = c.get('start', '?')
                    txt = (c.get('text') or '')[:200]
                    self.view.chunks_tree.insert('', 'end', iid=f'chunk_{i}', text=f'[{start}] {txt}', values=(start, txt))
            else:
                self.view.chunks_tree.insert('', 'end', text='(no chunks)')
        except Exception:
            pass

        # populate gaps tree
        try:
            for ch in self.view.gaps_tree.get_children():
                self.view.gaps_tree.delete(ch)
            if gaps:
                for i, g in enumerate(gaps):
                    gtype = g.get('type', g.get('gap_type', 'unknown'))
                    sev = g.get('severity', '')
                    desc = (g.get('description') or g.get('desc') or g.get('message') or '')[:300]
                    self.view.gaps_tree.insert('', 'end', iid=f'gap_{i}', text=gtype, values=(gtype, sev, desc))
            else:
                self.view.gaps_tree.insert('', 'end', text='(no gaps)')
        except Exception:
            pass

    def apply_filters(self):
        """Submit filter work to background thread and return immediately.

        Actual UI updates are applied asynchronously on the main thread from
        the result queue polled via `view.after`.
        """
        # collect parameters
        q = (self.view.search_var.get() or "").strip()
        gap_filter = (self.view.filter_gap_var.get() or "(any)")
        selected_indices = []
        try:
            selected_indices = list(self.view.filter_gap_listbox.curselection())
        except Exception:
            selected_indices = []
        severity = (self.view.filter_severity_var.get() or "(any)")
        regex = bool(self.view.regex_var.get())

        # indicate work in progress
        try:
            self.view.status.config(text="Filtering...")
        except Exception:
            pass

        # background job
        def job():
            ql = q.lower()
            # compute selected gap-type strings
            sel = []
            try:
                for i in selected_indices:
                    sel.append(self.view.filter_gap_listbox.get(i))
            except Exception:
                sel = []

            candidates = []
            for nid, node in self.model.nodes.items():
                label = (node.label or "")
                if ql and ql not in label.lower() and ql not in nid.lower():
                    continue

                gaps = self.model.gaps.get(nid) or self.model.gaps.get(node.label) or []
                if gap_filter and gap_filter != "(any)":
                    if not any(((g.get('type') or g.get('gap_type')) == gap_filter) for g in gaps):
                        continue

                if sel:
                    if not any(((g.get('type') or g.get('gap_type')) in sel) for g in gaps):
                        continue

                if severity and severity != "(any)":
                    if not any(str((g.get('severity') or '')).upper() == severity for g in gaps):
                        continue

                candidates.append((nid, node, gaps))

            # sort by current sort selection (read on main thread is fine)
            sort_key = (self.view.sort_var.get() or "label")
            if sort_key == "label":
                candidates.sort(key=lambda t: (t[1].label or "").lower())
            elif sort_key == "gap_count":
                candidates.sort(key=lambda t: -len(t[2]))
            elif sort_key == "chunk_count":
                candidates.sort(key=lambda t: -len(self.model.chunks.get(t[0]) or []))

            rows = [(t[0], t[1].label, len(t[2]), (t[2][0].get('type') if t[2] else '')) for t in candidates]

            return {"visible": [t[0] for t in candidates], "rows": rows, "count": len(candidates)}

        # submit job and enqueue result when done
        try:
            future = self._filter_executor.submit(job)
            def when_done(fut):
                try:
                    res = fut.result()
                    self._result_queue.put(res)
                except Exception:
                    self._result_queue.put({"visible": [], "rows": [], "count": 0})
            future.add_done_callback(when_done)
        except Exception:
            # fallback synchronous
            res = job()
            self._result_queue.put(res)

    def _poll_results(self):
        try:
            while not self._result_queue.empty():
                res = self._result_queue.get_nowait()
                try:
                    self._handle_filter_result(res)
                except Exception:
                    pass
        except Exception:
            pass
        finally:
            try:
                self.view.after(100, self._poll_results)
            except Exception:
                pass

    def _handle_filter_result(self, res: dict):
        vis = res.get('visible', [])
        rows = res.get('rows', [])
        self.visible_nodes = vis

        # update node list UI
        try:
            # clear tree
            for it in self.view.node_list.get_children():
                self.view.node_list.delete(it)
            for nid, label, cnt, first in rows:
                display = f"{label} ({cnt})" if cnt else label
                try:
                    self.view.node_list.insert('', 'end', iid=nid, text=display)
                except Exception:
                    # fallback: plain insert without iid
                    self.view.node_list.insert('', 'end', text=display)
        except Exception:
            pass

        # recompute layout and request redraw (coalesced)
        try:
            self._compute_layout()
            self.request_redraw()
        except Exception:
            pass

        try:
            self.view.status.config(text=f"Filtered: {len(vis)} nodes")
        except Exception:
            pass

    def export_visible_json(self):
        # write visible nodes and associated gaps/chunks
        data = {}
        for nid in self.visible_nodes:
            node = self.model.nodes.get(nid)
            data[nid] = {
                'label': node.label if node else nid,
                'meta': node.meta if node else {},
                'chunks': self.model.chunks.get(nid) or [],
                'gaps': self.model.gaps.get(nid) or [],
            }
        from tkinter import filedialog
        p = filedialog.asksaveasfilename(defaultextension='.json', filetypes=[('JSON','*.json')], title='Export Visible JSON')
        if p:
            with open(p, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
            self.view.status.config(text=f"Exported visible JSON to {p}")

    def export_visible_csv(self):
        # flatten visible nodes to CSV: id,label,gap_count,first_gap_type
        rows = []
        for nid in self.visible_nodes:
            node = self.model.nodes.get(nid)
            gaps = self.model.gaps.get(nid) or []
            first = (gaps[0].get('type') if gaps else '')
            rows.append((nid, node.label if node else nid, len(gaps), first))
        from tkinter import filedialog
        p = filedialog.asksaveasfilename(defaultextension='.csv', filetypes=[('CSV','*.csv')], title='Export Visible CSV')
        if p:
            import csv
            with open(p, 'w', newline='', encoding='utf-8') as f:
                w = csv.writer(f)
                w.writerow(['id','label','gap_count','first_gap_type'])
                for r in rows:
                    w.writerow(r)
            self.view.status.config(text=f"Exported visible CSV to {p}")
            
    def export_canvas_png(self):
        # Export the canvas area to PNG using Pillow ImageGrab (screen capture)
        from tkinter import filedialog
        if not HAS_PIL:
            messagebox.showinfo("Pillow required", "Export to PNG requires Pillow (pip install pillow)")
            return
        p = filedialog.asksaveasfilename(defaultextension='.png', filetypes=[('PNG','*.png')], title='Export Canvas PNG')
        if not p:
            return
        try:
            c = self.view.canvas
            # get canvas position on screen
            x = c.winfo_rootx()
            y = c.winfo_rooty()
            w = c.winfo_width()
            h = c.winfo_height()
            bbox = (x, y, x + w, y + h)
            img = ImageGrab.grab(bbox)
            img.save(p)
            self.view.status.config(text=f"Exported PNG to {p}")
        except Exception as e:
            messagebox.showerror("Export failed", str(e))

    def save_session(self):
        from tkinter import filedialog
        p = filedialog.asksaveasfilename(defaultextension='.json', filetypes=[('JSON','*.json')], title='Save Session')
        if not p:
            return
        try:
            data = {
                'visible_nodes': self.visible_nodes,
                'node_positions': self.node_positions,
                'target_positions': self._target_positions,
                'mode': getattr(self.view, 'mode_var', tk.StringVar(value='force')).get(),
                'cluster_key': getattr(self.view, 'cluster_key_var', tk.StringVar(value='topdir')).get(),
                'clustering': bool(getattr(self.view, 'clustering_var', tk.BooleanVar(value=False)).get()),
                'heatmap': bool(getattr(self.view, 'heatmap_var', tk.BooleanVar(value=False)).get()),
                'node_size': float(getattr(self.view, 'node_size_var', tk.DoubleVar(value=1.0)).get()),
                'color_map': getattr(self.view, 'color_map_var', tk.StringVar(value='rg')).get(),
            }
            with open(p, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2)
            self.view.status.config(text=f"Saved session to {p}")
        except Exception as e:
            messagebox.showerror("Save failed", str(e))

    def load_session(self):
        from tkinter import filedialog
        p = filedialog.askopenfilename(title="Load Session JSON", filetypes=[('JSON','*.json')])
        if not p:
            return
        try:
            with open(p, 'r', encoding='utf-8') as f:
                data = json.load(f)
            self.visible_nodes = data.get('visible_nodes') or self.visible_nodes
            # restore positions
            np = data.get('node_positions') or {}
            for k, v in np.items():
                try:
                    self.node_positions[k] = tuple(v)
                except Exception:
                    pass
            tp = data.get('target_positions') or {}
            for k, v in tp.items():
                try:
                    self._target_positions[k] = tuple(v)
                except Exception:
                    pass
            # restore UI state
            try:
                if 'mode' in data and hasattr(self.view, 'mode_var'):
                    self.view.mode_var.set(data.get('mode'))
                if 'cluster_key' in data and hasattr(self.view, 'cluster_key_var'):
                    self.view.cluster_key_var.set(data.get('cluster_key'))
                if 'clustering' in data and hasattr(self.view, 'clustering_var'):
                    self.view.clustering_var.set(bool(data.get('clustering')))
                if 'heatmap' in data and hasattr(self.view, 'heatmap_var'):
                    self.view.heatmap_var.set(bool(data.get('heatmap')))
                if 'node_size' in data and hasattr(self.view, 'node_size_var'):
                    self.view.node_size_var.set(float(data.get('node_size')))
                if 'color_map' in data and hasattr(self.view, 'color_map_var'):
                    self.view.color_map_var.set(data.get('color_map'))
                    try:
                        self._update_legend_preview()
                    except Exception:
                        pass
            except Exception:
                pass

            # recompute layout and request redraw
            try:
                self._compute_layout()
                self.request_redraw()
                self.view.status.config(text=f"Loaded session from {p}")
            except Exception:
                pass
        except Exception as e:
            messagebox.showerror("Load failed", str(e))


def main(argv=None):
    parser = argparse.ArgumentParser(description="Visualize graph, chunks and gaps (Tkinter)")
    parser.add_argument("--graph", required=True, help="Path to graph JSON file")
    parser.add_argument("--scan", required=False, help="Optional scan JSON to enrich gaps (path)")
    args = parser.parse_args(argv)

    view = VisualizerView()
    app = AppController(None, view)

    # show the window first, then load data in background to avoid blocking UI
    def start_loading():
        try:
            if getattr(view, 'progress', None):
                try:
                    view.progress.pack(side=tk.RIGHT, padx=(6,8))
                    view.progress.start(20)
                except Exception:
                    pass
            view.status.config(text='Loading data...')
        except Exception:
            pass

        def load_job():
            logging.debug('Background: starting load of graph JSON')
            try:
                with open(args.graph, 'r', encoding='utf-8') as f:
                    graph_data = json.load(f)
            except Exception as e:
                logging.exception('Failed to load graph file')
                return {'error': str(e)}

            if args.scan:
                try:
                    with open(args.scan, 'r', encoding='utf-8') as f:
                        scan = json.load(f)
                    gaps_list = scan.get('gaps', []) or []
                    gaps_map = graph_data.get('gaps', {}) or {}
                    if not isinstance(gaps_map, dict):
                        gaps_map = {}
                    for g in gaps_list:
                        fn = g.get('file') or g.get('path') or g.get('id')
                        if not fn:
                            continue
                        gaps_map.setdefault(fn, []).append(g)
                    graph_data['gaps'] = gaps_map
                except Exception:
                    logging.exception('Failed to merge scan JSON')

            try:
                model = GraphModel(graph_data)
                return {'model': model}
            except Exception as e:
                logging.exception('Failed to build GraphModel')
                return {'error': str(e)}

        import concurrent.futures
        ex = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        fut = ex.submit(load_job)

        def on_done(fut):
            res = None
            try:
                res = fut.result()
            except Exception as e:
                logging.exception('Background load failed')
                res = {'error': str(e)}

            def apply_result():
                try:
                    if getattr(view, 'progress', None):
                        try:
                            view.progress.stop()
                            view.progress.pack_forget()
                        except Exception:
                            pass
                    if not res:
                        view.status.config(text='Load failed (no result)')
                        return
                    if 'error' in res:
                        view.status.config(text=f"Load error: {res['error']}")
                        return
                    model = res.get('model')
                    if model:
                        app.set_model(model)
                        view.status.config(text='Data loaded')
                    else:
                        view.status.config(text='Load failed')
                except Exception:
                    logging.exception('apply_result failed')

            try:
                view.after(50, apply_result)
            except Exception:
                apply_result()

        fut.add_done_callback(on_done)

    # schedule loading shortly after mainloop starts so UI appears first
    try:
        view.after(100, start_loading)
    except Exception:
        start_loading()

    view.mainloop()


if __name__ == "__main__":
    main()
