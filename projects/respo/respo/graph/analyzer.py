"""
RESPO Code Graph Analyzer

Analyzes code to extract relationships for ThemisDB's graph capabilities:
- Import relationships
- Function calls
- Class inheritance
- Variable usage
- Module dependencies
"""

import ast
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Optional

import structlog

logger = structlog.get_logger(__name__)


@dataclass
class GraphNode:
    """Represents a code entity node."""
    
    id: str
    node_type: str  # function, class, module, method, variable
    name: str
    qualified_name: str
    file_path: str
    line_start: int
    line_end: int
    properties: dict[str, Any] = field(default_factory=dict)


@dataclass
class GraphEdge:
    """Represents a relationship between code entities."""
    
    source_id: str
    target_id: str
    edge_type: str  # imports, calls, inherits, implements, uses, defines, contains
    properties: dict[str, Any] = field(default_factory=dict)


@dataclass
class CodeGraph:
    """Graph representation of code relationships."""
    
    nodes: list[GraphNode] = field(default_factory=list)
    edges: list[GraphEdge] = field(default_factory=list)
    
    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            "nodes": [
                {
                    "id": n.id,
                    "type": n.node_type,
                    "name": n.name,
                    "qualified_name": n.qualified_name,
                    "file_path": n.file_path,
                    "line_start": n.line_start,
                    "line_end": n.line_end,
                    "properties": n.properties,
                }
                for n in self.nodes
            ],
            "edges": [
                {
                    "source": e.source_id,
                    "target": e.target_id,
                    "type": e.edge_type,
                    "properties": e.properties,
                }
                for e in self.edges
            ],
        }


class PythonGraphAnalyzer(ast.NodeVisitor):
    """
    AST-based analyzer for Python code to extract graph relationships.
    
    Extracts:
    - Module imports
    - Function definitions and calls
    - Class definitions and inheritance
    - Method definitions
    - Variable assignments and usages
    """
    
    def __init__(self, file_path: str, module_name: str) -> None:
        """
        Initialize the analyzer.
        
        Args:
            file_path: Path to the source file
            module_name: Module name for qualified names
        """
        self.file_path = file_path
        self.module_name = module_name
        self.nodes: list[GraphNode] = []
        self.edges: list[GraphEdge] = []
        
        # Tracking context
        self._current_class: Optional[str] = None
        self._current_function: Optional[str] = None
        self._scope_stack: list[str] = []
        
        # Track defined names for resolving references
        self._defined_names: dict[str, str] = {}  # name -> node_id
        self._imported_names: dict[str, str] = {}  # name -> module
    
    def _make_id(self, name: str) -> str:
        """Generate a unique ID for a node."""
        if self._scope_stack:
            qualified = f"{self.module_name}.{'.'.join(self._scope_stack)}.{name}"
        else:
            qualified = f"{self.module_name}.{name}"
        return qualified
    
    def _current_scope_id(self) -> str:
        """Get the ID of the current scope."""
        if self._scope_stack:
            return f"{self.module_name}.{'.'.join(self._scope_stack)}"
        return self.module_name
    
    def analyze(self, source: str) -> CodeGraph:
        """
        Analyze Python source code.
        
        Args:
            source: Python source code
            
        Returns:
            CodeGraph with nodes and edges
        """
        try:
            tree = ast.parse(source)
            
            # Add module node
            module_node = GraphNode(
                id=self.module_name,
                node_type="module",
                name=self.module_name.split(".")[-1],
                qualified_name=self.module_name,
                file_path=self.file_path,
                line_start=1,
                line_end=len(source.splitlines()),
            )
            self.nodes.append(module_node)
            
            # Visit all nodes
            self.visit(tree)
            
        except SyntaxError as e:
            logger.warning("Syntax error in code", error=str(e), file=self.file_path)
        
        return CodeGraph(nodes=self.nodes, edges=self.edges)
    
    def visit_Import(self, node: ast.Import) -> None:
        """Handle import statements."""
        for alias in node.names:
            module_name = alias.name
            local_name = alias.asname or module_name.split(".")[0]
            
            self._imported_names[local_name] = module_name
            
            # Create edge: current module imports target module
            self.edges.append(GraphEdge(
                source_id=self.module_name,
                target_id=module_name,
                edge_type="imports",
                properties={"alias": alias.asname, "line": node.lineno},
            ))
        
        self.generic_visit(node)
    
    def visit_ImportFrom(self, node: ast.ImportFrom) -> None:
        """Handle from ... import statements."""
        module = node.module or ""
        
        for alias in node.names:
            if alias.name == "*":
                # Star import - import the whole module
                self.edges.append(GraphEdge(
                    source_id=self.module_name,
                    target_id=module,
                    edge_type="imports",
                    properties={"star": True, "line": node.lineno},
                ))
            else:
                name = alias.name
                local_name = alias.asname or name
                full_name = f"{module}.{name}" if module else name
                
                self._imported_names[local_name] = full_name
                
                self.edges.append(GraphEdge(
                    source_id=self.module_name,
                    target_id=full_name,
                    edge_type="imports",
                    properties={"from": module, "alias": alias.asname, "line": node.lineno},
                ))
        
        self.generic_visit(node)
    
    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        """Handle class definitions."""
        class_id = self._make_id(node.name)
        
        # Create class node
        class_node = GraphNode(
            id=class_id,
            node_type="class",
            name=node.name,
            qualified_name=class_id,
            file_path=self.file_path,
            line_start=node.lineno,
            line_end=node.end_lineno or node.lineno,
            properties={
                "decorators": [self._get_decorator_name(d) for d in node.decorator_list],
                "docstring": ast.get_docstring(node),
            },
        )
        self.nodes.append(class_node)
        self._defined_names[node.name] = class_id
        
        # Add contains edge from module/class to this class
        self.edges.append(GraphEdge(
            source_id=self._current_scope_id(),
            target_id=class_id,
            edge_type="contains",
        ))
        
        # Handle inheritance
        for base in node.bases:
            base_name = self._get_name(base)
            if base_name:
                # Resolve to full name if imported
                base_full = self._imported_names.get(base_name, base_name)
                self.edges.append(GraphEdge(
                    source_id=class_id,
                    target_id=base_full,
                    edge_type="inherits",
                ))
        
        # Visit class body
        self._scope_stack.append(node.name)
        old_class = self._current_class
        self._current_class = class_id
        
        self.generic_visit(node)
        
        self._current_class = old_class
        self._scope_stack.pop()
    
    def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
        """Handle function definitions."""
        self._handle_function(node)
    
    def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
        """Handle async function definitions."""
        self._handle_function(node, is_async=True)
    
    def _handle_function(
        self, 
        node: ast.FunctionDef | ast.AsyncFunctionDef,
        is_async: bool = False,
    ) -> None:
        """Handle function/method definition."""
        func_id = self._make_id(node.name)
        
        # Determine if method or function
        node_type = "method" if self._current_class else "function"
        
        # Create function node
        func_node = GraphNode(
            id=func_id,
            node_type=node_type,
            name=node.name,
            qualified_name=func_id,
            file_path=self.file_path,
            line_start=node.lineno,
            line_end=node.end_lineno or node.lineno,
            properties={
                "is_async": is_async,
                "decorators": [self._get_decorator_name(d) for d in node.decorator_list],
                "docstring": ast.get_docstring(node),
                "args": [arg.arg for arg in node.args.args],
                "returns": self._get_annotation(node.returns) if node.returns else None,
            },
        )
        self.nodes.append(func_node)
        self._defined_names[node.name] = func_id
        
        # Add contains/defines edge
        self.edges.append(GraphEdge(
            source_id=self._current_scope_id(),
            target_id=func_id,
            edge_type="defines" if self._current_class else "contains",
        ))
        
        # Visit function body to find calls
        self._scope_stack.append(node.name)
        old_function = self._current_function
        self._current_function = func_id
        
        self.generic_visit(node)
        
        self._current_function = old_function
        self._scope_stack.pop()
    
    def visit_Call(self, node: ast.Call) -> None:
        """Handle function calls."""
        if self._current_function:
            call_name = self._get_name(node.func)
            if call_name:
                # Resolve to full name if defined locally or imported
                target = self._defined_names.get(
                    call_name,
                    self._imported_names.get(call_name, call_name)
                )
                
                self.edges.append(GraphEdge(
                    source_id=self._current_function,
                    target_id=target,
                    edge_type="calls",
                    properties={"line": node.lineno},
                ))
        
        self.generic_visit(node)
    
    def visit_Attribute(self, node: ast.Attribute) -> None:
        """Handle attribute access (method calls on objects)."""
        # Check if this is a method call (parent is Call)
        # We handle this through visit_Call, so skip here
        self.generic_visit(node)
    
    def visit_Name(self, node: ast.Name) -> None:
        """Handle name references."""
        if self._current_function and isinstance(node.ctx, ast.Load):
            name = node.id
            # Check if it's a known name
            if name in self._imported_names:
                self.edges.append(GraphEdge(
                    source_id=self._current_function,
                    target_id=self._imported_names[name],
                    edge_type="uses",
                    properties={"line": node.lineno},
                ))
            elif name in self._defined_names:
                self.edges.append(GraphEdge(
                    source_id=self._current_function,
                    target_id=self._defined_names[name],
                    edge_type="uses",
                    properties={"line": node.lineno},
                ))
        
        self.generic_visit(node)
    
    def _get_name(self, node: ast.expr) -> Optional[str]:
        """Extract name from an AST node."""
        if isinstance(node, ast.Name):
            return node.id
        elif isinstance(node, ast.Attribute):
            value = self._get_name(node.value)
            if value:
                return f"{value}.{node.attr}"
            return node.attr
        elif isinstance(node, ast.Subscript):
            return self._get_name(node.value)
        return None
    
    def _get_decorator_name(self, node: ast.expr) -> str:
        """Get decorator name."""
        name = self._get_name(node)
        if name:
            return name
        if isinstance(node, ast.Call):
            return self._get_name(node.func) or "<unknown>"
        return "<unknown>"
    
    def _get_annotation(self, node: ast.expr) -> str:
        """Get type annotation as string."""
        try:
            return ast.unparse(node)
        except Exception:
            return str(node)


class JavaScriptGraphAnalyzer:
    """
    Regex-based analyzer for JavaScript/TypeScript code.
    
    Extracts:
    - Import statements (ES6, CommonJS)
    - Function definitions
    - Class definitions and inheritance
    - Method definitions
    - Export statements
    """
    
    def __init__(self, file_path: str, module_name: str) -> None:
        """
        Initialize the analyzer.
        
        Args:
            file_path: Path to the source file
            module_name: Module name for qualified names
        """
        self.file_path = file_path
        self.module_name = module_name
        self.nodes: list[GraphNode] = []
        self.edges: list[GraphEdge] = []
    
    def analyze(self, source: str) -> CodeGraph:
        """
        Analyze JavaScript/TypeScript source code.
        
        Args:
            source: Source code
            
        Returns:
            CodeGraph with nodes and edges
        """
        lines = source.splitlines()
        
        # Add module node
        self.nodes.append(GraphNode(
            id=self.module_name,
            node_type="module",
            name=self.module_name.split("/")[-1],
            qualified_name=self.module_name,
            file_path=self.file_path,
            line_start=1,
            line_end=len(lines),
        ))
        
        # Parse imports
        self._extract_imports(source)
        
        # Parse classes
        self._extract_classes(source)
        
        # Parse functions
        self._extract_functions(source)
        
        return CodeGraph(nodes=self.nodes, edges=self.edges)
    
    def _extract_imports(self, source: str) -> None:
        """Extract import statements."""
        # ES6 imports
        es6_import = re.compile(
            r'import\s+(?:'
            r'(?:(?P<default>[\w$]+)\s*,?\s*)?'
            r'(?:\{(?P<named>[^}]+)\}\s*)?'
            r'(?:\*\s+as\s+(?P<namespace>[\w$]+)\s*)?'
            r')?\s*from\s*[\'"](?P<module>[^"\']+)[\'"]',
            re.MULTILINE
        )
        
        for match in es6_import.finditer(source):
            module = match.group("module")
            self.edges.append(GraphEdge(
                source_id=self.module_name,
                target_id=module,
                edge_type="imports",
                properties={
                    "default": match.group("default"),
                    "named": match.group("named"),
                    "namespace": match.group("namespace"),
                },
            ))
        
        # CommonJS require
        require_pattern = re.compile(
            r'(?:const|let|var)\s+(?:(?P<name>[\w$]+)|{(?P<destructure>[^}]+)})\s*='
            r'\s*require\([\'"](?P<module>[^"\']+)[\'"]\)',
            re.MULTILINE
        )
        
        for match in require_pattern.finditer(source):
            module = match.group("module")
            self.edges.append(GraphEdge(
                source_id=self.module_name,
                target_id=module,
                edge_type="imports",
                properties={"commonjs": True},
            ))
    
    def _extract_classes(self, source: str) -> None:
        """Extract class definitions."""
        class_pattern = re.compile(
            r'(?:export\s+)?(?:default\s+)?class\s+(?P<name>[\w$]+)'
            r'(?:\s+extends\s+(?P<extends>[\w$.]+))?'
            r'(?:\s+implements\s+(?P<implements>[\w$.,\s]+))?',
            re.MULTILINE
        )
        
        for match in class_pattern.finditer(source):
            class_name = match.group("name")
            class_id = f"{self.module_name}.{class_name}"
            
            # Find line number
            line_num = source[:match.start()].count("\n") + 1
            
            self.nodes.append(GraphNode(
                id=class_id,
                node_type="class",
                name=class_name,
                qualified_name=class_id,
                file_path=self.file_path,
                line_start=line_num,
                line_end=line_num,  # Approximate
            ))
            
            self.edges.append(GraphEdge(
                source_id=self.module_name,
                target_id=class_id,
                edge_type="contains",
            ))
            
            # Handle extends
            extends = match.group("extends")
            if extends:
                self.edges.append(GraphEdge(
                    source_id=class_id,
                    target_id=extends,
                    edge_type="inherits",
                ))
            
            # Handle implements
            implements = match.group("implements")
            if implements:
                for interface in implements.split(","):
                    interface = interface.strip()
                    self.edges.append(GraphEdge(
                        source_id=class_id,
                        target_id=interface,
                        edge_type="implements",
                    ))
    
    def _extract_functions(self, source: str) -> None:
        """Extract function definitions."""
        # Regular functions
        func_pattern = re.compile(
            r'(?:export\s+)?(?:async\s+)?function\s+(?P<name>[\w$]+)\s*\(',
            re.MULTILINE
        )
        
        for match in func_pattern.finditer(source):
            func_name = match.group("name")
            func_id = f"{self.module_name}.{func_name}"
            line_num = source[:match.start()].count("\n") + 1
            
            self.nodes.append(GraphNode(
                id=func_id,
                node_type="function",
                name=func_name,
                qualified_name=func_id,
                file_path=self.file_path,
                line_start=line_num,
                line_end=line_num,
            ))
            
            self.edges.append(GraphEdge(
                source_id=self.module_name,
                target_id=func_id,
                edge_type="contains",
            ))
        
        # Arrow functions assigned to variables
        arrow_pattern = re.compile(
            r'(?:export\s+)?(?:const|let|var)\s+(?P<name>[\w$]+)\s*=\s*(?:async\s+)?'
            r'(?:\([^)]*\)|[\w$]+)\s*=>',
            re.MULTILINE
        )
        
        for match in arrow_pattern.finditer(source):
            func_name = match.group("name")
            func_id = f"{self.module_name}.{func_name}"
            line_num = source[:match.start()].count("\n") + 1
            
            self.nodes.append(GraphNode(
                id=func_id,
                node_type="function",
                name=func_name,
                qualified_name=func_id,
                file_path=self.file_path,
                line_start=line_num,
                line_end=line_num,
                properties={"arrow": True},
            ))
            
            self.edges.append(GraphEdge(
                source_id=self.module_name,
                target_id=func_id,
                edge_type="contains",
            ))


class CodeGraphAnalyzer:
    """
    Main interface for code graph analysis.
    
    Supports multiple languages and provides a unified interface
    for extracting code relationships for ThemisDB's graph features.
    """
    
    LANGUAGE_ANALYZERS = {
        "python": PythonGraphAnalyzer,
        "javascript": JavaScriptGraphAnalyzer,
        "typescript": JavaScriptGraphAnalyzer,
    }
    
    def __init__(self) -> None:
        """Initialize the analyzer."""
        pass
    
    def analyze(
        self,
        source: str,
        language: str,
        file_path: str,
        module_name: Optional[str] = None,
    ) -> CodeGraph:
        """
        Analyze source code and extract graph relationships.
        
        Args:
            source: Source code content
            language: Programming language
            file_path: Path to the file
            module_name: Module name (default: derived from file_path)
            
        Returns:
            CodeGraph with nodes and edges
        """
        if module_name is None:
            module_name = self._path_to_module(file_path, language)
        
        analyzer_class = self.LANGUAGE_ANALYZERS.get(language.lower())
        
        if analyzer_class is None:
            logger.warning(
                "No graph analyzer for language",
                language=language,
                file=file_path,
            )
            # Return minimal graph
            return CodeGraph(
                nodes=[
                    GraphNode(
                        id=module_name,
                        node_type="module",
                        name=Path(file_path).stem,
                        qualified_name=module_name,
                        file_path=file_path,
                        line_start=1,
                        line_end=len(source.splitlines()),
                    )
                ],
                edges=[],
            )
        
        analyzer = analyzer_class(file_path, module_name)
        return analyzer.analyze(source)
    
    def _path_to_module(self, file_path: str, language: str) -> str:
        """Convert file path to module name."""
        path = Path(file_path)
        
        if language in ("python",):
            # Python: src/package/module.py -> src.package.module
            parts = list(path.with_suffix("").parts)
            return ".".join(parts)
        else:
            # JavaScript/TypeScript: src/utils/helper.ts -> src/utils/helper
            return str(path.with_suffix(""))
    
    def analyze_directory(
        self,
        directory: Path,
        languages: Optional[list[str]] = None,
    ) -> CodeGraph:
        """
        Analyze all code files in a directory.
        
        Args:
            directory: Directory to analyze
            languages: Languages to include (default: all supported)
            
        Returns:
            Combined CodeGraph
        """
        all_nodes: list[GraphNode] = []
        all_edges: list[GraphEdge] = []
        
        extensions = {
            "python": [".py", ".pyi"],
            "javascript": [".js", ".jsx", ".mjs"],
            "typescript": [".ts", ".tsx"],
        }
        
        languages = languages or list(self.LANGUAGE_ANALYZERS.keys())
        
        for lang in languages:
            if lang not in extensions:
                continue
            
            for ext in extensions[lang]:
                for file_path in directory.rglob(f"*{ext}"):
                    try:
                        content = file_path.read_text(encoding="utf-8")
                        relative = str(file_path.relative_to(directory))
                        
                        graph = self.analyze(
                            source=content,
                            language=lang,
                            file_path=relative,
                        )
                        
                        all_nodes.extend(graph.nodes)
                        all_edges.extend(graph.edges)
                        
                    except (IOError, UnicodeDecodeError) as e:
                        logger.warning(
                            "Could not read file",
                            path=str(file_path),
                            error=str(e),
                        )
        
        return CodeGraph(nodes=all_nodes, edges=all_edges)
