#!/usr/bin/env python3
"""
Smart Header Format Detector for ThemisDB

Recognizes and preserves existing header formats:
- Copyright/License blocks
- File description blocks
- Custom gap statistics formats
- Only updates the stats, preserves the layout
"""

import re
from pathlib import Path
from typing import Optional, Dict, List

class HeaderFormatDetector:
    """Detect and preserve existing header formats"""
    
    # Known header patterns
    PATTERNS = {
        'copyright_block': re.compile(r'^/\*[\s\S]*?\*/', re.MULTILINE),
        'license_block': re.compile(r'^// (Copyright|License|SPDX|LGPL|GPL|MIT)', re.MULTILINE),
        'file_description': re.compile(r'^//\s+@file|^//\s+\w+\s+—|^//\s+\w+:', re.MULTILINE),
        'themis_gap_stats': re.compile(r'^// THEMIS_GAP_STATS:', re.MULTILINE),
        'themis_gap_analysis': re.compile(r'^// THEMIS_GAP_ANALYSIS', re.MULTILINE),
    }
    
    def __init__(self, file_path: Path):
        self.file_path = file_path
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                self.content = f.read()
        except:
            self.content = ""
        
        self.lines = self.content.split('\n')
        self._analyze()
    
    def _analyze(self):
        """Analyze the header structure"""
        self.header_type = None
        self.header_end_line = 0
        self.existing_gap_line = None
        
        # Find where the header ends
        for i, line in enumerate(self.lines):
            if line.strip() == '':
                continue
            if line.startswith('//') or line.startswith('/*'):
                self.header_end_line = i + 1
            else:
                # Found first non-comment line
                break
        
        # Determine header type
        header_text = '\n'.join(self.lines[:self.header_end_line])
        
        if self.PATTERNS['copyright_block'].search(header_text):
            self.header_type = 'copyright'
        elif self.PATTERNS['license_block'].search(header_text):
            self.header_type = 'license'
        elif self.PATTERNS['file_description'].search(header_text):
            self.header_type = 'description'
        elif self.PATTERNS['themis_gap_stats'].search(header_text):
            self.header_type = 'themis_stats'
        elif self.PATTERNS['themis_gap_analysis'].search(header_text):
            self.header_type = 'themis_analysis'
        else:
            self.header_type = 'generic_comments'
        
        # Find existing THEMIS_GAP line if present
        for i, line in enumerate(self.lines):
            if 'THEMIS_GAP_STATS' in line:
                self.existing_gap_line = i
                break
    
    def get_header_type(self) -> str:
        """Return detected header type"""
        return self.header_type
    
    def get_header_end_line(self) -> int:
        """Return line number where header ends"""
        return self.header_end_line
    
    def has_themis_gap_header(self) -> bool:
        """Check if file has existing THEMIS_GAP header"""
        return self.existing_gap_line is not None
    
    def get_existing_gap_line_number(self) -> Optional[int]:
        """Get line number of existing THEMIS_GAP header"""
        return self.existing_gap_line
    
    def insert_or_update_gap_stats(self, gap_stats_line: str) -> str:
        """
        Insert or update gap stats while preserving existing header.
        
        Rules:
        - If THEMIS_GAP header exists: update that line
        - If copyright/license exists: insert after it
        - If description exists: insert after it
        - Otherwise: insert at top
        """
        
        lines = self.content.split('\n')
        
        if self.existing_gap_line is not None:
            # Update existing line
            lines[self.existing_gap_line] = gap_stats_line
            return '\n'.join(lines)
        
        # Insert new header
        insert_pos = self.header_end_line
        
        # If there's an existing header, add separator
        if insert_pos > 0 and lines[insert_pos - 1].startswith('//'):
            # Insert after header block
            lines.insert(insert_pos, gap_stats_line)
        else:
            # Insert at beginning
            lines.insert(0, gap_stats_line)
        
        return '\n'.join(lines)
    
    def print_analysis(self):
        """Print detected header structure"""
        print(f"📄 File: {self.file_path.name}")
        print(f"   Header Type: {self.header_type}")
        print(f"   Header End Line: {self.header_end_line}")
        print(f"   Has THEMIS_GAP: {self.has_themis_gap_header()}")
        if self.existing_gap_line:
            print(f"   THEMIS_GAP Line: {self.existing_gap_line + 1}")

def analyze_repo_headers(repo_root: str) -> Dict[str, int]:
    """Analyze all headers in repo"""
    
    repo_path = Path(repo_root)
    stats = {
        'total_files': 0,
        'has_copyright': 0,
        'has_license': 0,
        'has_description': 0,
        'has_themis_stats': 0,
        'has_themis_analysis': 0,
        'has_generic_comments': 0,
        'no_header': 0,
    }
    
    for cpp_file in repo_path.glob('**/*.cpp'):
        if 'build' in cpp_file.parts or '.venv' in cpp_file.parts:
            continue
        
        stats['total_files'] += 1
        detector = HeaderFormatDetector(cpp_file)
        header_type = detector.get_header_type()
        
        if header_type in stats:
            stats[header_type] += 1
        else:
            stats['no_header'] += 1
    
    for h_file in repo_path.glob('**/*.hpp'):
        if 'build' in h_file.parts or '.venv' in h_file.parts:
            continue
        
        stats['total_files'] += 1
        detector = HeaderFormatDetector(h_file)
        header_type = detector.get_header_type()
        
        if header_type in stats:
            stats[header_type] += 1
        else:
            stats['no_header'] += 1
    
    return stats

if __name__ == '__main__':
    import sys
    
    if len(sys.argv) > 1:
        file_path = Path(sys.argv[1])
        detector = HeaderFormatDetector(file_path)
        detector.print_analysis()
    else:
        # Analyze repo
        repo_root = '.'
        if len(sys.argv) > 2:
            repo_root = sys.argv[2]
        
        print("🔍 Header Format Analysis")
        print("=" * 60)
        
        stats = analyze_repo_headers(repo_root)
        
        print(f"\n📊 Summary:")
        print(f"   Total Files: {stats['total_files']}")
        print(f"   Copyright blocks: {stats['has_copyright']}")
        print(f"   License blocks: {stats['has_license']}")
        print(f"   File descriptions: {stats['has_description']}")
        print(f"   THEMIS_GAP_STATS: {stats['has_themis_stats']}")
        print(f"   THEMIS_GAP_ANALYSIS: {stats['has_themis_analysis']}")
        print(f"   Generic comments: {stats['has_generic_comments']}")
        print(f"   No header: {stats['no_header']}")
