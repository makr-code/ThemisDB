#!/usr/bin/env python3
"""
ThemisDB File Header Gap Statistics Generator

Adds/updates file headers with gap statistics:
// THEMIS_GAP_STATS: gaps=5 unimplemented=3 stub=2 todo=0 simulations=2 last_scanned=2026-05-18
"""

import re
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional
from dataclasses import dataclass

@dataclass
class FileGapStats:
    """Gap statistics for a single file"""
    total: int = 0
    unimplemented: int = 0
    stub_documented: int = 0
    stub_undocumented: int = 0
    mock: int = 0
    simulation: int = 0
    todo: int = 0
    fixme: int = 0
    xxx: int = 0
    technical_debt: int = 0
    platform_specific: int = 0
    disabled: int = 0
    
    def format_header(self) -> str:
        """Format as a header comment line"""
        return (
            f"// THEMIS_GAP_STATS: "
            f"gaps={self.total} "
            f"unimpl={self.unimplemented} "
            f"stub={self.stub_documented+self.stub_undocumented} "
            f"mock={self.mock} "
            f"sim={self.simulation} "
            f"todo={self.todo} "
            f"debt={self.technical_debt} "
            f"scanned={datetime.now().strftime('%Y-%m-%d')}"
        )
    
    def format_detailed_header(self) -> str:
        """Format as multi-line detailed header"""
        return (
            f"// THEMIS_GAP_ANALYSIS\n"
            f"//   Total Gaps: {self.total}\n"
            f"//   Unimplemented: {self.unimplemented}\n"
            f"//   STUB (documented): {self.stub_documented}\n"
            f"//   STUB (undocumented): {self.stub_undocumented}\n"
            f"//   Mock/Test: {self.mock}\n"
            f"//   Simulations: {self.simulation}\n"
            f"//   TODO/FIXME: {self.todo + self.fixme}\n"
            f"//   Technical Debt: {self.technical_debt}\n"
            f"//   Platform-Specific: {self.platform_specific}\n"
            f"//   Disabled Code: {self.disabled}\n"
            f"//   Last Scanned: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
        )

class FileHeaderUpdater:
    """Update file headers with gap statistics, preserving existing formats"""
    
    # Patterns to recognize existing THEMIS_GAP headers
    HEADER_STATS_PATTERN = re.compile(
        r'^(// THEMIS_GAP_STATS:.*?)$',
        re.MULTILINE
    )
    
    HEADER_ANALYSIS_PATTERN = re.compile(
        r'^(// THEMIS_GAP_ANALYSIS\n(?://.*\n)*)',
        re.MULTILINE
    )
    
    # Detect existing header blocks (copyright, description, etc.)
    EXISTING_HEADER_BLOCK = re.compile(
        r'^((?://.*\n)*)',  # Capture all leading comment lines
        re.MULTILINE
    )
    
    def __init__(self):
        pass
    
    def _detect_header_format(self, content: str) -> Optional[str]:
        """Detect which header format is already in the file"""
        if self.HEADER_STATS_PATTERN.search(content):
            return 'stats'
        elif self.HEADER_ANALYSIS_PATTERN.search(content):
            return 'analysis'
        return None
    
    def _extract_existing_header_block(self, content: str) -> tuple[str, str]:
        """
        Extract existing header block (before any code).
        Returns (header_block, rest_of_content)
        """
        match = self.EXISTING_HEADER_BLOCK.match(content)
        if match:
            header_block = match.group(1)
            rest = content[len(header_block):]
            return header_block, rest
        return '', content
    
    def update_file_header(self, file_path: Path, stats: FileGapStats, 
                          detailed: bool = False) -> bool:
        """
        Update or add gap statistics header to file.
        Preserves existing header format if present.
        
        Returns True if file was modified, False otherwise.
        """
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except:
            return False
        
        original_content = content
        
        # Detect existing header format
        existing_format = self._detect_header_format(content)
        
        if existing_format == 'stats':
            # Update existing STATS header (single line)
            new_header = stats.format_header() + '\n'
            new_content = self.HEADER_STATS_PATTERN.sub(new_header.rstrip('\n'), content, count=1)
        
        elif existing_format == 'analysis':
            # Update existing ANALYSIS header (multi-line)
            new_header = stats.format_detailed_header()
            new_content = self.HEADER_ANALYSIS_PATTERN.sub(new_header, content, count=1)
        
        else:
            # No existing THEMIS_GAP header, add new one
            # But preserve any existing header block (copyright, etc.)
            header_block, rest = self._extract_existing_header_block(content)
            
            if detailed:
                new_header = stats.format_detailed_header()
            else:
                new_header = stats.format_header() + '\n'
            
            # Insert THEMIS_GAP header after existing header block
            if header_block:
                # Skip trailing empty comment lines
                header_block = header_block.rstrip()
                if header_block:
                    new_content = header_block + '\n//\n' + new_header + rest
                else:
                    new_content = new_header + rest
            else:
                new_content = new_header + rest
        
        # Only write if changed
        if new_content != original_content:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                return True
            except:
                return False
        
        return False
    
    def get_file_header(self, file_path: Path) -> Optional[str]:
        """Extract existing THEMIS_GAP header from file"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except:
            return None
        
        # Try to match STATS format
        match = self.HEADER_STATS_PATTERN.search(content)
        if match:
            return match.group(1)
        
        # Try to match ANALYSIS format
        match = self.HEADER_ANALYSIS_PATTERN.search(content)
        if match:
            return match.group(1)
        
        return None
    
    def batch_update_files(self, gap_report: Dict[str, Dict], 
                          repo_root: str, detailed: bool = False,
                          max_files: int = None) -> Dict[str, int]:
        """
        Batch update all files based on gap report.
        Preserves existing header formats.
        
        gap_report format:
        {
            'gap_scan_v2_<module>.json': {
                'gaps_by_file': {
                    'src/module/file.cpp': [gaps...],
                    ...
                }
            }
        }
        
        max_files: limit number of files to update (for testing)
        """
        stats = {
            'total_files': 0,
            'files_updated': 0,
            'files_with_gaps': 0,
            'files_no_gaps': 0,
            'files_skipped': 0
        }
        
        repo_path = Path(repo_root)
        
        # Flatten all gap data from report
        all_files_gaps = {}
        for module_data in gap_report.values():
            if 'gaps_by_file' not in module_data:
                continue
            all_files_gaps.update(module_data['gaps_by_file'])
        
        # Import smart detector
        from header_format_detector import HeaderFormatDetector
        
        file_count = 0
        
        # Process all source files in repo
        for cpp_file in sorted(repo_path.glob('**/*.cpp')):
            if 'build' in cpp_file.parts or '.venv' in cpp_file.parts:
                continue
            
            if max_files and file_count >= max_files:
                stats['files_skipped'] = len(list(repo_path.glob('**/*.cpp'))) - file_count
                break
            
            file_count += 1
            stats['total_files'] += 1
            
            # Get stats for this file
            file_rel = str(cpp_file.relative_to(repo_path))
            file_gaps = all_files_gaps.get(file_rel, [])
            
            # Calculate statistics
            file_stats = self._calculate_stats(file_gaps)
            
            if file_stats.total > 0:
                stats['files_with_gaps'] += 1
                if self.update_file_header(cpp_file, file_stats, detailed):
                    stats['files_updated'] += 1
            else:
                stats['files_no_gaps'] += 1
        
        for h_file in sorted(repo_path.glob('**/*.hpp')):
            if 'build' in h_file.parts or '.venv' in h_file.parts:
                continue
            
            if max_files and file_count >= max_files:
                stats['files_skipped'] = len(list(repo_path.glob('**/*.hpp'))) - (file_count - stats['total_files'])
                break
            
            file_count += 1
            stats['total_files'] += 1
            
            file_rel = str(h_file.relative_to(repo_path))
            file_gaps = all_files_gaps.get(file_rel, [])
            
            file_stats = self._calculate_stats(file_gaps)
            
            if file_stats.total > 0:
                stats['files_with_gaps'] += 1
                if self.update_file_header(h_file, file_stats, detailed):
                    stats['files_updated'] += 1
            else:
                stats['files_no_gaps'] += 1
        
        return stats
    
    def _calculate_stats(self, gaps: List[Dict]) -> FileGapStats:
        """Calculate statistics from gap list"""
        stats = FileGapStats()
        
        for gap in gaps:
            stats.total += 1
            category = gap.get('category', 'unknown')
            
            if category == 'unimplemented':
                stats.unimplemented += 1
            elif category == 'stub_documented':
                stats.stub_documented += 1
            elif category == 'stub_undocumented':
                stats.stub_undocumented += 1
            elif category == 'mock_framework':
                stats.mock += 1
            elif category == 'simulation':
                stats.simulation += 1
            elif category == 'todo_item':
                stats.todo += 1
            elif category == 'fixme_item':
                stats.fixme += 1
            elif category == 'xxx_item':
                stats.xxx += 1
            elif category == 'technical_debt':
                stats.technical_debt += 1
            elif category == 'platform_fallback':
                stats.platform_specific += 1
            elif category == 'disabled_code':
                stats.disabled += 1
        
        return stats


if __name__ == '__main__':
    import json
    import sys
    
    # Example usage
    if len(sys.argv) > 1:
        gap_report_file = sys.argv[1]
        repo_root = sys.argv[2] if len(sys.argv) > 2 else '.'
        detailed = '--detailed' in sys.argv
        
        with open(gap_report_file) as f:
            gap_report = json.load(f)
        
        updater = FileHeaderUpdater()
        result = updater.batch_update_files(gap_report, repo_root, detailed)
        
        print(f"[OK] Header Update Complete:")
        print(f"   Total Files: {result['total_files']}")
        print(f"   Updated: {result['files_updated']}")
        print(f"   No Gaps: {result['files_no_gaps']}")
