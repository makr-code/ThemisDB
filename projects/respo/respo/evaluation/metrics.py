"""
Code Quality Metrics

Static analysis metrics for code quality assessment.
"""

import ast
import re
from dataclasses import dataclass
from typing import Optional


@dataclass
class CodeQualityMetrics:
    """Static code quality metrics."""
    
    # Size metrics
    lines_of_code: int
    lines_of_comments: int
    blank_lines: int
    total_lines: int
    
    # Complexity metrics
    cyclomatic_complexity: int
    cognitive_complexity: int
    max_nesting_depth: int
    
    # Function metrics
    num_functions: int
    num_classes: int
    avg_function_length: float
    max_function_length: int
    
    # Documentation
    has_docstrings: bool
    docstring_coverage: float  # 0-1
    
    # Code smells
    long_functions: int  # > 50 lines
    deep_nesting: int  # > 4 levels
    many_parameters: int  # > 5 params
    
    # Naming
    snake_case_functions: int
    camel_case_functions: int
    
    def to_dict(self) -> dict:
        """Convert to dictionary."""
        return {
            "lines_of_code": self.lines_of_code,
            "lines_of_comments": self.lines_of_comments,
            "blank_lines": self.blank_lines,
            "total_lines": self.total_lines,
            "cyclomatic_complexity": self.cyclomatic_complexity,
            "cognitive_complexity": self.cognitive_complexity,
            "max_nesting_depth": self.max_nesting_depth,
            "num_functions": self.num_functions,
            "num_classes": self.num_classes,
            "avg_function_length": self.avg_function_length,
            "max_function_length": self.max_function_length,
            "has_docstrings": self.has_docstrings,
            "docstring_coverage": self.docstring_coverage,
            "long_functions": self.long_functions,
            "deep_nesting": self.deep_nesting,
            "many_parameters": self.many_parameters,
            "snake_case_functions": self.snake_case_functions,
            "camel_case_functions": self.camel_case_functions,
        }
    
    def quality_score(self) -> float:
        """
        Calculate an overall quality score (0-10).
        
        Based on various metrics weighted by importance.
        """
        score = 10.0
        
        # Penalize high complexity
        if self.cyclomatic_complexity > 10:
            score -= min(2.0, (self.cyclomatic_complexity - 10) * 0.2)
        
        # Penalize deep nesting
        if self.max_nesting_depth > 4:
            score -= min(1.5, (self.max_nesting_depth - 4) * 0.5)
        
        # Penalize long functions
        if self.long_functions > 0:
            score -= min(1.5, self.long_functions * 0.5)
        
        # Penalize many parameters
        if self.many_parameters > 0:
            score -= min(1.0, self.many_parameters * 0.3)
        
        # Reward documentation
        if self.has_docstrings:
            score += 0.5
        score += self.docstring_coverage * 1.0
        
        # Reward good comment ratio
        if self.lines_of_code > 0:
            comment_ratio = self.lines_of_comments / self.lines_of_code
            if 0.1 <= comment_ratio <= 0.3:
                score += 0.5
        
        return max(0.0, min(10.0, score))


def calculate_code_metrics(code: str, language: str = "python") -> Optional[CodeQualityMetrics]:
    """
    Calculate code quality metrics.
    
    Args:
        code: Source code to analyze
        language: Programming language
    
    Returns:
        CodeQualityMetrics or None if parsing fails
    """
    if language.lower() != "python":
        # For non-Python, return basic metrics
        return _calculate_basic_metrics(code)
    
    return _calculate_python_metrics(code)


def _calculate_basic_metrics(code: str) -> CodeQualityMetrics:
    """Calculate basic metrics for any language."""
    lines = code.split("\n")
    total_lines = len(lines)
    blank_lines = sum(1 for line in lines if not line.strip())
    
    # Simple comment detection
    comment_patterns = [
        r'^\s*#',      # Python, Shell
        r'^\s*//',     # C, Java, JS
        r'^\s*/\*',    # Multi-line start
        r'^\s*\*',     # Multi-line middle
    ]
    
    lines_of_comments = 0
    for line in lines:
        for pattern in comment_patterns:
            if re.match(pattern, line):
                lines_of_comments += 1
                break
    
    lines_of_code = total_lines - blank_lines - lines_of_comments
    
    # Simple function detection
    function_patterns = [
        r'\bdef\s+\w+',           # Python
        r'\bfunction\s+\w+',      # JS
        r'\bfunc\s+\w+',          # Go
        r'\bfn\s+\w+',            # Rust
        r'\b(public|private|protected)?\s*(static)?\s*\w+\s+\w+\s*\(',  # Java/C#
    ]
    
    num_functions = 0
    for pattern in function_patterns:
        num_functions += len(re.findall(pattern, code))
    
    return CodeQualityMetrics(
        lines_of_code=lines_of_code,
        lines_of_comments=lines_of_comments,
        blank_lines=blank_lines,
        total_lines=total_lines,
        cyclomatic_complexity=1,  # Unknown
        cognitive_complexity=1,
        max_nesting_depth=0,
        num_functions=num_functions,
        num_classes=len(re.findall(r'\bclass\s+\w+', code)),
        avg_function_length=lines_of_code / max(1, num_functions),
        max_function_length=0,
        has_docstrings=False,
        docstring_coverage=0.0,
        long_functions=0,
        deep_nesting=0,
        many_parameters=0,
        snake_case_functions=0,
        camel_case_functions=0,
    )


def _calculate_python_metrics(code: str) -> Optional[CodeQualityMetrics]:
    """Calculate detailed metrics for Python code."""
    try:
        tree = ast.parse(code)
    except SyntaxError:
        return _calculate_basic_metrics(code)
    
    lines = code.split("\n")
    total_lines = len(lines)
    blank_lines = sum(1 for line in lines if not line.strip())
    lines_of_comments = sum(1 for line in lines if line.strip().startswith("#"))
    lines_of_code = total_lines - blank_lines - lines_of_comments
    
    # Collect function info
    functions = []
    classes = []
    
    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef) or isinstance(node, ast.AsyncFunctionDef):
            functions.append(node)
        elif isinstance(node, ast.ClassDef):
            classes.append(node)
    
    # Calculate complexity
    cyclomatic = _calculate_cyclomatic_complexity(tree)
    cognitive = _calculate_cognitive_complexity(tree)
    max_depth = _calculate_max_nesting(tree)
    
    # Function lengths
    function_lengths = []
    for func in functions:
        if hasattr(func, 'end_lineno') and hasattr(func, 'lineno'):
            length = func.end_lineno - func.lineno + 1
            function_lengths.append(length)
    
    avg_func_length = sum(function_lengths) / len(function_lengths) if function_lengths else 0
    max_func_length = max(function_lengths) if function_lengths else 0
    
    # Docstrings
    docstrings = 0
    for node in functions + classes:
        if (node.body and isinstance(node.body[0], ast.Expr) and 
            isinstance(node.body[0].value, ast.Constant) and 
            isinstance(node.body[0].value.value, str)):
            docstrings += 1
    
    total_documentable = len(functions) + len(classes)
    docstring_coverage = docstrings / total_documentable if total_documentable > 0 else 0
    
    # Code smells
    long_functions = sum(1 for l in function_lengths if l > 50)
    many_parameters = sum(1 for f in functions if len(f.args.args) > 5)
    
    # Naming conventions
    snake_case = sum(1 for f in functions if re.match(r'^[a-z_][a-z0-9_]*$', f.name))
    camel_case = sum(1 for f in functions if re.match(r'^[a-z][a-zA-Z0-9]*$', f.name) and '_' not in f.name)
    
    return CodeQualityMetrics(
        lines_of_code=lines_of_code,
        lines_of_comments=lines_of_comments,
        blank_lines=blank_lines,
        total_lines=total_lines,
        cyclomatic_complexity=cyclomatic,
        cognitive_complexity=cognitive,
        max_nesting_depth=max_depth,
        num_functions=len(functions),
        num_classes=len(classes),
        avg_function_length=avg_func_length,
        max_function_length=max_func_length,
        has_docstrings=docstrings > 0,
        docstring_coverage=docstring_coverage,
        long_functions=long_functions,
        deep_nesting=1 if max_depth > 4 else 0,
        many_parameters=many_parameters,
        snake_case_functions=snake_case,
        camel_case_functions=camel_case,
    )


def _calculate_cyclomatic_complexity(tree: ast.AST) -> int:
    """Calculate cyclomatic complexity."""
    complexity = 1  # Base complexity
    
    for node in ast.walk(tree):
        if isinstance(node, (ast.If, ast.While, ast.For, ast.AsyncFor)):
            complexity += 1
        elif isinstance(node, ast.ExceptHandler):
            complexity += 1
        elif isinstance(node, ast.BoolOp):
            complexity += len(node.values) - 1
        elif isinstance(node, (ast.Assert, ast.comprehension)):
            complexity += 1
    
    return complexity


def _calculate_cognitive_complexity(tree: ast.AST) -> int:
    """Calculate cognitive complexity (simplified)."""
    complexity = 0
    nesting = 0
    
    class ComplexityVisitor(ast.NodeVisitor):
        def __init__(self):
            self.complexity = 0
            self.nesting = 0
        
        def visit_If(self, node):
            self.complexity += 1 + self.nesting
            self.nesting += 1
            self.generic_visit(node)
            self.nesting -= 1
        
        def visit_For(self, node):
            self.complexity += 1 + self.nesting
            self.nesting += 1
            self.generic_visit(node)
            self.nesting -= 1
        
        def visit_While(self, node):
            self.complexity += 1 + self.nesting
            self.nesting += 1
            self.generic_visit(node)
            self.nesting -= 1
        
        def visit_Try(self, node):
            self.complexity += 1
            self.nesting += 1
            self.generic_visit(node)
            self.nesting -= 1
        
        def visit_BoolOp(self, node):
            self.complexity += len(node.values) - 1
            self.generic_visit(node)
    
    visitor = ComplexityVisitor()
    visitor.visit(tree)
    return visitor.complexity


def _calculate_max_nesting(tree: ast.AST) -> int:
    """Calculate maximum nesting depth."""
    max_depth = 0
    
    class NestingVisitor(ast.NodeVisitor):
        def __init__(self):
            self.max_depth = 0
            self.current_depth = 0
        
        def visit_If(self, node):
            self.current_depth += 1
            self.max_depth = max(self.max_depth, self.current_depth)
            self.generic_visit(node)
            self.current_depth -= 1
        
        def visit_For(self, node):
            self.current_depth += 1
            self.max_depth = max(self.max_depth, self.current_depth)
            self.generic_visit(node)
            self.current_depth -= 1
        
        def visit_While(self, node):
            self.current_depth += 1
            self.max_depth = max(self.max_depth, self.current_depth)
            self.generic_visit(node)
            self.current_depth -= 1
        
        def visit_Try(self, node):
            self.current_depth += 1
            self.max_depth = max(self.max_depth, self.current_depth)
            self.generic_visit(node)
            self.current_depth -= 1
        
        def visit_With(self, node):
            self.current_depth += 1
            self.max_depth = max(self.max_depth, self.current_depth)
            self.generic_visit(node)
            self.current_depth -= 1
    
    visitor = NestingVisitor()
    visitor.visit(tree)
    return visitor.max_depth


def calculate_similarity(code_a: str, code_b: str) -> dict:
    """
    Calculate similarity between two code snippets.
    
    Returns various similarity metrics.
    """
    # Normalize code
    def normalize(code: str) -> str:
        # Remove comments
        code = re.sub(r'#.*$', '', code, flags=re.MULTILINE)
        code = re.sub(r'//.*$', '', code, flags=re.MULTILINE)
        code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
        # Normalize whitespace
        code = re.sub(r'\s+', ' ', code)
        return code.strip().lower()
    
    norm_a = normalize(code_a)
    norm_b = normalize(code_b)
    
    # Exact match
    exact_match = norm_a == norm_b
    
    # Token-based similarity (Jaccard)
    tokens_a = set(re.findall(r'\w+', norm_a))
    tokens_b = set(re.findall(r'\w+', norm_b))
    
    if not tokens_a or not tokens_b:
        jaccard = 0.0
    else:
        intersection = len(tokens_a & tokens_b)
        union = len(tokens_a | tokens_b)
        jaccard = intersection / union if union > 0 else 0.0
    
    # Line-based similarity
    lines_a = set(code_a.strip().split('\n'))
    lines_b = set(code_b.strip().split('\n'))
    
    if not lines_a or not lines_b:
        line_similarity = 0.0
    else:
        line_intersection = len(lines_a & lines_b)
        line_union = len(lines_a | lines_b)
        line_similarity = line_intersection / line_union if line_union > 0 else 0.0
    
    # Length ratio
    len_a = len(code_a)
    len_b = len(code_b)
    length_ratio = min(len_a, len_b) / max(len_a, len_b) if max(len_a, len_b) > 0 else 1.0
    
    return {
        "exact_match": exact_match,
        "jaccard_similarity": jaccard,
        "line_similarity": line_similarity,
        "length_ratio": length_ratio,
        "overall_similarity": (jaccard + line_similarity + length_ratio) / 3,
    }
