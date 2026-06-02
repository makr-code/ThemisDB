#!/usr/bin/env python3
"""
ThemisDB Gap Scanner — Progress & Logging Utilities

Provides structured logging, progress tracking, and formatted output for all scanners.
"""

import sys
import time
import os
from datetime import datetime
from typing import Optional, Dict, List
from pathlib import Path

# Fix Windows console encoding for Unicode
if sys.platform == 'win32':
    os.environ['PYTHONIOENCODING'] = 'utf-8'
    # Try to set UTF-8 mode
    if hasattr(sys.stdout, 'reconfigure'):
        try:
            sys.stdout.reconfigure(encoding='utf-8')
        except:
            pass


class ProgressLogger:
    """Structured logging with progress tracking for gap scanners."""
    
    # ANSI Color Codes
    COLORS = {
        'RESET': '\033[0m',
        'BOLD': '\033[1m',
        'DIM': '\033[2m',
        'CYAN': '\033[36m',
        'GREEN': '\033[32m',
        'YELLOW': '\033[33m',
        'RED': '\033[31m',
        'BLUE': '\033[34m',
        'MAGENTA': '\033[35m',
    }
    
    # Status Symbols (with ASCII fallbacks)
    SYMBOLS = {
        'OK': '✓',          # ASCII fallback: [OK]
        'SKIP': '⊘',        # ASCII fallback: [SKIP]
        'PROGRESS': '…',    # ASCII fallback: [...]
        'FAIL': '✗',        # ASCII fallback: [FAIL]
        'WARN': '⚠',        # ASCII fallback: [WARN]
        'INFO': 'ℹ',        # ASCII fallback: [INFO]
    }
    
    SYMBOLS_ASCII = {
        'OK': '[OK]',
        'SKIP': '[SKIP]',
        'PROGRESS': '[...]',
        'FAIL': '[FAIL]',
        'WARN': '[WARN]',
        'INFO': '[INFO]',
    }
    
    def __init__(self, verbose: bool = True, use_colors: bool = True, use_unicode: bool = True):
        self.verbose = verbose
        self.use_colors = use_colors
        self.use_unicode = use_unicode
        self.start_time = time.time()
        self.stage_times: Dict[str, float] = {}
        self.current_stage: Optional[str] = None
        self.stage_start_time: Optional[float] = None
    
    def _get_symbol(self, symbol_key: str) -> str:
        """Get symbol with fallback to ASCII."""
        if self.use_unicode:
            try:
                return self.SYMBOLS.get(symbol_key, '•')
            except UnicodeEncodeError:
                return self.SYMBOLS_ASCII.get(symbol_key, '[?]')
        else:
            return self.SYMBOLS_ASCII.get(symbol_key, '[?]')
    
    def _normalize_text(self, text: str) -> str:
        """Normalize Unicode characters to ASCII if needed."""
        if not self.use_unicode:
            text = text.replace("—", "-")
            text = text.replace("–", "-")
        return text
    
    def _colorize(self, text: str, color: str) -> str:
        """Add color to text if colors are enabled."""
        text = self._normalize_text(text)
        if not self.use_colors or not sys.stdout.isatty():
            return text
        return f"{self.COLORS.get(color, '')}{text}{self.COLORS['RESET']}"
    
    def _elapsed(self) -> str:
        """Return formatted elapsed time."""
        elapsed = time.time() - self.start_time
        minutes, seconds = divmod(int(elapsed), 60)
        return f"{minutes}m{seconds}s" if minutes > 0 else f"{seconds}s"
    
    def _stage_elapsed(self) -> str:
        """Return formatted stage elapsed time."""
        if not self.stage_start_time:
            return "0s"
        elapsed = time.time() - self.stage_start_time
        return f"{elapsed:.1f}s"
    
    def separator(self) -> None:
        """Print a separator line."""
        print(self._colorize("=" * 90, "BLUE"))
    
    def header(self, title: str) -> None:
        """Print a formatted header."""
        self.separator()
        print(self._colorize(f"  {title}", "BOLD"))
        self.separator()
    
    def stage_start(self, stage_name: str, description: str = "") -> None:
        """Mark start of a new stage."""
        self.current_stage = stage_name
        self.stage_start_time = time.time()
        
        msg = f"{self._get_symbol('PROGRESS')} [{stage_name}]"
        if description:
            msg += f" - {description}"
        print(f"\n{self._colorize(msg, 'CYAN')}")
    
    def stage_complete(self, message: str = "") -> None:
        """Mark completion of current stage."""
        if not self.current_stage:
            return
        
        elapsed = self._stage_elapsed()
        msg = f"{self._get_symbol('OK')} [{self.current_stage}] Complete ({elapsed})"
        if message:
            msg += f" - {message}"
        
        print(self._colorize(msg, "GREEN"))
        
        if self.current_stage:
            self.stage_times[self.current_stage] = time.time() - (self.stage_start_time or time.time())
    
    def step(self, step_name: str, message: str = "", status: str = "OK") -> None:
        """Log a single step with status."""
        symbol = self._get_symbol(status)
        color = {
            'OK': 'GREEN',
            'SKIP': 'YELLOW',
            'FAIL': 'RED',
            'WARN': 'YELLOW',
            'INFO': 'BLUE',
        }.get(status, 'RESET')
        
        msg = f"  {symbol} {step_name}"
        if message:
            msg += f" - {message}"
        
        print(self._colorize(msg, color))
    
    def progress(self, current: int, total: int, item_name: str = "", show_pct: bool = True) -> None:
        """Log progress indicator."""
        pct = (current / total * 100) if total > 0 else 0
        bar_length = 40
        filled = int(bar_length * current / total) if total > 0 else 0
        bar = '█' * filled + '░' * (bar_length - filled) if self.use_unicode else ('[' + '#' * filled + '-' * (bar_length - filled) + ']')
        
        pct_str = f" {pct:3.0f}%" if show_pct else ""
        item_str = f" ({item_name})" if item_name else ""
        
        msg = f"  ⊡ [{bar}] {current}/{total}{pct_str}{item_str}" if self.use_unicode else f"  > [{bar}] {current}/{total}{pct_str}{item_str}"
        print(self._colorize(msg, "MAGENTA"), end='\r')
        
        if current == total:
            print()  # Newline when complete
    
    def summary(self, title: str, items: List[tuple], show_total: bool = True) -> None:
        """Print a summary table."""
        print(f"\n{self._colorize(title, 'BOLD')}")
        print(self._colorize("-" * 90, "DIM"))
        
        total = 0
        for label, value in items:
            # Format value with thousands separator if numeric
            if isinstance(value, int):
                val_str = f"{value:,}"
                total += value
            else:
                val_str = str(value)
            
            print(f"  {label:<40} {val_str:>20}")
        
        if show_total and total > 0:
            print(self._colorize("-" * 90, "DIM"))
            print(f"  {'TOTAL':<40} {total:>20,}")
    
    def timing_summary(self, title: str = "Pipeline Timing") -> None:
        """Print timing summary."""
        if not self.stage_times:
            return
        
        print(f"\n{self._colorize(title, 'BOLD')}")
        print(self._colorize("-" * 90, "DIM"))
        
        sorted_stages = sorted(self.stage_times.items(), key=lambda x: -x[1])
        for stage, duration in sorted_stages:
            print(f"  {stage:<40} {duration:>15.1f}s")
        
        total_time = self._elapsed()
        print(self._colorize("-" * 90, "DIM"))
        print(f"  {'Total Elapsed':<40} {total_time:>20}")
    
    def error(self, title: str, message: str = "", details: str = "") -> None:
        """Log an error."""
        print(f"\n{self._get_symbol('FAIL')} {self._colorize(title, 'RED')}")
        if message:
            print(f"   {message}")
        if details:
            print(f"   {details}")
    
    def warning(self, title: str, message: str = "") -> None:
        """Log a warning."""
        msg = f"{self._get_symbol('WARN')} {title}"
        if message:
            msg += f" - {message}"
        print(self._colorize(msg, "YELLOW"))
    
    def info(self, title: str, message: str = "") -> None:
        """Log an info message."""
        msg = f"{self._get_symbol('INFO')} {title}"
        if message:
            msg += f" - {message}"
        print(self._colorize(msg, "BLUE"))
    
    def module_result(self, module_name: str, total: int, categories: Dict[str, int], 
                      severity_breakdown: Dict[str, int]) -> None:
        """Log results for a single module scan."""
        crit = severity_breakdown.get('critical', 0)
        high = severity_breakdown.get('high', 0)
        medium = severity_breakdown.get('medium', 0)
        
        color = 'RED' if crit > 0 else ('YELLOW' if high > 0 else 'GREEN')
        symbol = self._get_symbol('OK') if total == 0 else self._get_symbol('WARN')
        
        msg = f"  {symbol} {module_name:<30} {total:>5} findings"
        msg += f" ({crit} crit, {high} high, {medium} med)"
        
        print(self._colorize(msg, color))


def create_logger(verbose: bool = True, use_colors: bool = True, use_unicode: bool = None) -> ProgressLogger:
    """Factory for creating a ProgressLogger instance.
    
    Args:
        verbose: Enable verbose output
        use_colors: Enable ANSI color codes
        use_unicode: Use Unicode symbols (auto-detect if None)
    """
    # Auto-detect: use ASCII on Windows by default, Unicode on others
    if use_unicode is None:
        use_unicode = sys.platform != 'win32'
    
    return ProgressLogger(verbose=verbose, use_colors=use_colors, use_unicode=use_unicode)


if __name__ == '__main__':
    # Demo usage
    logger = create_logger()
    
    logger.header("Gap Scanner v3.1 Demo")
    
    logger.stage_start("INIT", "Initializing scanner environment")
    logger.step("Config", "Loading configuration from CMakePresets.json", "OK")
    logger.step("Paths", "Validating src/ include/ structure", "OK")
    logger.stage_complete("All systems ready")
    
    logger.stage_start("SCAN", "Running phase 1-4 core scanners")
    for i in range(1, 11):
        logger.progress(i, 10, f"Module {i}/10")
        time.sleep(0.1)
    logger.stage_complete("Scanned 64 modules")
    
    logger.stage_start("AGGREGATE", "Consolidating results")
    logger.step("Summary", "Calculating per-module statistics", "OK")
    logger.step("Confidence", "Scoring and ranking findings", "OK")
    logger.stage_complete()
    
    logger.summary("Gap Statistics", [
        ("Total Findings", 27990),
        ("Critical", 3904),
        ("High", 11008),
        ("Medium", 17415),
        ("Top Module", "llm (4,719)"),
        ("Top Category", "container (7,374)"),
    ])
    
    logger.timing_summary()
