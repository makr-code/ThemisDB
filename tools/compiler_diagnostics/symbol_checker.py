"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            symbol_checker.py                                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     335                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41ed  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Symbol Checker - Validate symbol visibility and exports

This tool uses nm (Unix) or dumpbin (Windows) to verify that symbols
are properly exported/imported in compiled binaries.

Usage:
    python symbol_checker.py <binary_path> [--platform {windows|linux|macos}]
"""

import re
import subprocess
import argparse
from pathlib import Path
from typing import List, Dict, Set, Tuple
from dataclasses import dataclass
from collections import defaultdict

try:
    from . import THEMIS_ROOT
except ImportError:
    THEMIS_ROOT = Path(__file__).parent.parent.parent


@dataclass
class Symbol:
    """Represents a symbol in a binary"""
    name: str
    symbol_type: str  # T (text), U (undefined), D (data), etc.
    visibility: str  # default, hidden, protected
    demangled_name: str = ""


class SymbolChecker:
    """Check symbol visibility in binaries"""
    
    def __init__(self, platform: str = "auto"):
        self.platform = platform
        if platform == "auto":
            self.platform = self._detect_platform()
    
    def _detect_platform(self) -> str:
        """Detect current platform"""
        import platform as plat
        system = plat.system().lower()
        if system == "windows":
            return "windows"
        elif system == "darwin":
            return "macos"
        else:
            return "linux"
    
    def check_binary(self, binary_path: Path) -> Tuple[List[Symbol], Dict]:
        """Check symbols in a binary"""
        if self.platform == "windows":
            return self._check_windows(binary_path)
        else:
            return self._check_unix(binary_path)
    
    def _check_unix(self, binary_path: Path) -> Tuple[List[Symbol], Dict]:
        """Check symbols on Unix-like systems using nm"""
        symbols = []
        stats = defaultdict(int)
        
        try:
            # Run nm command
            result = subprocess.run(
                ['nm', '-C', '-D', str(binary_path)],
                capture_output=True,
                text=True,
                check=False
            )
            
            if result.returncode != 0:
                # Try without -D flag (for static libraries)
                result = subprocess.run(
                    ['nm', '-C', str(binary_path)],
                    capture_output=True,
                    text=True,
                    check=True
                )
            
            # Parse nm output
            # Format: [address] type name
            for line in result.stdout.split('\n'):
                if not line.strip():
                    continue
                
                parts = line.split()
                if len(parts) < 2:
                    continue
                
                # Handle lines with or without address
                if len(parts) == 2:
                    symbol_type, name = parts
                else:
                    symbol_type = parts[1]
                    name = ' '.join(parts[2:])
                
                # Determine visibility (default for exported, hidden otherwise)
                visibility = "default" if symbol_type in ['T', 'D', 'B'] else "hidden"
                
                symbol = Symbol(
                    name=name,
                    symbol_type=symbol_type,
                    visibility=visibility,
                    demangled_name=name  # nm -C already demangles
                )
                symbols.append(symbol)
                stats[f"type_{symbol_type}"] += 1
                stats[f"visibility_{visibility}"] += 1
            
        except FileNotFoundError:
            print("Error: 'nm' command not found. Please install binutils.")
            return [], {}
        except subprocess.CalledProcessError as e:
            print(f"Error running nm: {e}")
            return [], {}
        
        stats["total_symbols"] = len(symbols)
        return symbols, dict(stats)
    
    def _check_windows(self, binary_path: Path) -> Tuple[List[Symbol], Dict]:
        """Check symbols on Windows using dumpbin"""
        symbols = []
        stats = defaultdict(int)
        
        try:
            # Run dumpbin command
            result = subprocess.run(
                ['dumpbin', '/EXPORTS', str(binary_path)],
                capture_output=True,
                text=True,
                check=True
            )
            
            # Parse dumpbin output
            in_exports = False
            for line in result.stdout.split('\n'):
                line = line.strip()
                
                if 'ordinal' in line.lower() and 'name' in line.lower():
                    in_exports = True
                    continue
                
                if not in_exports:
                    continue
                
                if not line or line.startswith('Summary'):
                    break
                
                # Parse export line
                # Format: ordinal hint RVA name
                parts = line.split()
                if len(parts) >= 4:
                    name = parts[3]
                    symbol = Symbol(
                        name=name,
                        symbol_type='EXPORT',
                        visibility='default',
                        demangled_name=self._demangle_msvc(name)
                    )
                    symbols.append(symbol)
                    stats["type_EXPORT"] += 1
                    stats["visibility_default"] += 1
            
        except FileNotFoundError:
            print("Error: 'dumpbin' command not found. Please install Visual Studio.")
            return [], {}
        except subprocess.CalledProcessError as e:
            print(f"Error running dumpbin: {e}")
            return [], {}
        
        stats["total_symbols"] = len(symbols)
        return symbols, dict(stats)
    
    def _demangle_msvc(self, name: str) -> str:
        """Simple MSVC name demangling (basic)"""
        # This is a simplified version - full demangling requires undname.exe
        if name.startswith('?'):
            # Try to use undname if available
            try:
                result = subprocess.run(
                    ['undname', name],
                    capture_output=True,
                    text=True,
                    check=True,
                    timeout=1
                )
                return result.stdout.strip()
            except Exception:
                print(f"[WARN] Symbol lookup via subprocess failed for '{name}'")
        return name
    
    def verify_exports(self, binary_path: Path, expected_exports: Set[str]) -> Dict:
        """Verify that expected symbols are exported"""
        symbols, stats = self.check_binary(binary_path)
        
        exported_names = {s.name for s in symbols if s.visibility == 'default'}
        
        missing = expected_exports - exported_names
        unexpected = exported_names - expected_exports
        
        return {
            "total_exported": len(exported_names),
            "expected_count": len(expected_exports),
            "missing_exports": list(missing),
            "unexpected_exports": list(unexpected),
            "all_correct": len(missing) == 0
        }
    
    def report(self, binary_path: Path, output_path: Path) -> None:
        """Generate a report of symbol visibility"""
        symbols, stats = self.check_binary(binary_path)
        
        report = []
        report.append(f"# Symbol Visibility Report\n\n")
        report.append(f"Binary: {binary_path}\n")
        report.append(f"Platform: {self.platform}\n\n")
        
        report.append("## Statistics\n\n")
        for key, value in sorted(stats.items()):
            report.append(f"- **{key}**: {value}\n")
        report.append("\n")
        
        # Group symbols by type
        by_type = defaultdict(list)
        for symbol in symbols:
            by_type[symbol.symbol_type].append(symbol)
        
        report.append("## Symbols by Type\n\n")
        for symbol_type in sorted(by_type.keys()):
            symbol_list = by_type[symbol_type]
            report.append(f"### Type {symbol_type} ({len(symbol_list)} symbols)\n\n")
            
            # Show first 50 symbols
            for symbol in symbol_list[:50]:
                report.append(f"- {symbol.name}\n")
            
            if len(symbol_list) > 50:
                report.append(f"\n... and {len(symbol_list) - 50} more\n")
            report.append("\n")
        
        output_path.write_text(''.join(report))
        print(f"Report written to {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Check symbol visibility in binaries"
    )
    parser.add_argument(
        "binary",
        type=Path,
        help="Path to binary file (.so, .dll, .dylib, .a, .lib)"
    )
    parser.add_argument(
        "--platform",
        choices=["windows", "linux", "macos", "auto"],
        default="auto",
        help="Platform (auto-detect if not specified)"
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Output report file"
    )
    parser.add_argument(
        "--verify",
        type=Path,
        help="File containing expected exports (one per line)"
    )
    
    args = parser.parse_args()
    
    checker = SymbolChecker(platform=args.platform)
    
    if args.verify:
        # Verify mode
        expected = set(args.verify.read_text().strip().split('\n'))
        result = checker.verify_exports(args.binary, expected)
        
        print(f"\n=== Export Verification ===")
        print(f"Total exported: {result['total_exported']}")
        print(f"Expected: {result['expected_count']}")
        
        if result['missing_exports']:
            print(f"\nMissing {len(result['missing_exports'])} exports:")
            for name in result['missing_exports'][:20]:
                print(f"  - {name}")
        
        if result['unexpected_exports']:
            print(f"\nUnexpected {len(result['unexpected_exports'])} exports:")
            for name in result['unexpected_exports'][:20]:
                print(f"  - {name}")
        
        if result['all_correct']:
            print("\n✅ All expected symbols are correctly exported")
            return 0
        else:
            print("\n❌ Some symbols are missing or unexpected")
            return 1
    else:
        # Report mode
        output = args.output or Path(f"{args.binary.stem}_symbols.md")
        checker.report(args.binary, output)
        return 0


if __name__ == "__main__":
    exit(main())
