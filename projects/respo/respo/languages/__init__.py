"""
RESPO Multi-Language Support

Language-specific configurations for code analysis and processing.
"""

from dataclasses import dataclass
from typing import List, Optional


@dataclass
class LanguageConfig:
    """Configuration for a programming language."""
    
    name: str
    extensions: List[str]
    comment_single: str
    comment_multi_start: Optional[str]
    comment_multi_end: Optional[str]
    docstring_pattern: Optional[str]
    import_patterns: List[str]
    function_patterns: List[str]
    class_patterns: List[str]


# Language configurations
LANGUAGE_CONFIGS = {
    "python": LanguageConfig(
        name="Python",
        extensions=[".py", ".pyw", ".pyi"],
        comment_single="#",
        comment_multi_start='"""',
        comment_multi_end='"""',
        docstring_pattern=r'"""[\s\S]*?"""',
        import_patterns=[r"^import\s+(\w+)", r"^from\s+(\w+)\s+import"],
        function_patterns=[r"^def\s+(\w+)\s*\("],
        class_patterns=[r"^class\s+(\w+)\s*[\(:]"],
    ),
    "javascript": LanguageConfig(
        name="JavaScript",
        extensions=[".js", ".mjs", ".cjs"],
        comment_single="//",
        comment_multi_start="/*",
        comment_multi_end="*/",
        docstring_pattern=r"/\*\*[\s\S]*?\*/",
        import_patterns=[r"^import\s+.*\s+from\s+['\"](.+)['\"]", r"require\(['\"](.+)['\"]\)"],
        function_patterns=[r"function\s+(\w+)\s*\(", r"const\s+(\w+)\s*=\s*(?:async\s+)?\("],
        class_patterns=[r"class\s+(\w+)\s*(?:extends|{)"],
    ),
    "typescript": LanguageConfig(
        name="TypeScript",
        extensions=[".ts", ".tsx", ".mts", ".cts"],
        comment_single="//",
        comment_multi_start="/*",
        comment_multi_end="*/",
        docstring_pattern=r"/\*\*[\s\S]*?\*/",
        import_patterns=[r"^import\s+.*\s+from\s+['\"](.+)['\"]"],
        function_patterns=[r"function\s+(\w+)\s*[\(<]", r"const\s+(\w+)\s*=\s*(?:async\s+)?[\(<]"],
        class_patterns=[r"class\s+(\w+)\s*(?:extends|implements|{|<)"],
    ),
    "go": LanguageConfig(
        name="Go",
        extensions=[".go"],
        comment_single="//",
        comment_multi_start="/*",
        comment_multi_end="*/",
        docstring_pattern=None,
        import_patterns=[r'^import\s+"(.+)"', r'^import\s+\w+\s+"(.+)"'],
        function_patterns=[r"func\s+(?:\(\w+\s+\*?\w+\)\s+)?(\w+)\s*\("],
        class_patterns=[r"type\s+(\w+)\s+struct\s*{"],
    ),
    "rust": LanguageConfig(
        name="Rust",
        extensions=[".rs"],
        comment_single="//",
        comment_multi_start="/*",
        comment_multi_end="*/",
        docstring_pattern=r"///.*",
        import_patterns=[r"^use\s+(\w+)", r"^use\s+crate::(\w+)"],
        function_patterns=[r"fn\s+(\w+)\s*[\(<]"],
        class_patterns=[r"struct\s+(\w+)\s*[\(<{]", r"impl\s+(\w+)"],
    ),
    "java": LanguageConfig(
        name="Java",
        extensions=[".java"],
        comment_single="//",
        comment_multi_start="/*",
        comment_multi_end="*/",
        docstring_pattern=r"/\*\*[\s\S]*?\*/",
        import_patterns=[r"^import\s+([\w.]+);"],
        function_patterns=[r"(?:public|private|protected)?\s*(?:static\s+)?(?:\w+\s+)+(\w+)\s*\("],
        class_patterns=[r"class\s+(\w+)\s*(?:extends|implements|{)"],
    ),
}


def get_language_config(language: str) -> Optional[LanguageConfig]:
    """Get configuration for a programming language."""
    return LANGUAGE_CONFIGS.get(language.lower())


def detect_language(filename: str) -> Optional[str]:
    """Detect programming language from filename."""
    for lang, config in LANGUAGE_CONFIGS.items():
        for ext in config.extensions:
            if filename.endswith(ext):
                return lang
    return None


def get_supported_extensions() -> List[str]:
    """Get all supported file extensions."""
    extensions = []
    for config in LANGUAGE_CONFIGS.values():
        extensions.extend(config.extensions)
    return extensions
