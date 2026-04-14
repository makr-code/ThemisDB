#!/usr/bin/env python3
"""
Validate that all file references in Copilot instructions exist.
This prevents broken links in documentation.
"""

import os
import re
import sys
from pathlib import Path

def find_markdown_links(content):
    """Extract all markdown links from content."""
    # Matches [text](path) patterns
    pattern = r'\[([^\]]+)\]\(([^)]+)\)'
    return re.findall(pattern, content)

def is_external_link(path):
    """Check if link is external (http/https)."""
    return path.startswith(('http://', 'https://', '#', 'mailto:'))

def validate_file_reference(base_path, ref_path, source_file):
    """Validate that a referenced file exists."""
    # Remove any anchor fragments (#section)
    clean_path = ref_path.split('#')[0]
    
    if is_external_link(clean_path):
        return True, None
    
    # Empty path after removing anchor (internal link only)
    if not clean_path:
        return True, None
    
    # Always resolve relative to source file's directory
    source_dir = Path(source_file).parent
    full_path = (source_dir / clean_path).resolve()
    
    if not full_path.exists():
        return False, f"Referenced file does not exist: {clean_path}"
    
    return True, None

def validate_copilot_instructions():
    """Validate all Copilot instruction files."""
    repo_root = Path(__file__).parent.parent.parent
    copilot_dir = repo_root / '.github'
    
    errors = []
    files_checked = 0
    
    # Check the main instructions file using the repository's canonical path.
    main_file_candidates = [
        copilot_dir / 'copilot-instructions.md',
        copilot_dir / 'COPILOT_INSTRUCTIONS.md',
    ]
    main_file = next((path for path in main_file_candidates if path.exists()), None)
    if main_file is not None:
        with open(main_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        links = find_markdown_links(content)
        files_checked += 1
        
        for text, path in links:
            valid, error = validate_file_reference(repo_root, path, main_file)
            if not valid:
                errors.append(f"{main_file.relative_to(repo_root)}: {error}")
    
    # Check module files
    module_dir = copilot_dir / 'copilot'
    if module_dir.exists():
        for module_file in module_dir.glob('*.md'):
            with open(module_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            links = find_markdown_links(content)
            files_checked += 1
            
            for text, path in links:
                valid, error = validate_file_reference(repo_root, path, module_file)
                if not valid:
                    errors.append(f"{module_file.relative_to(repo_root)}: {error}")
    
    # Print results
    print(f"✓ Validated {files_checked} Copilot instruction files")
    
    if errors:
        print("\n❌ Validation errors found:")
        for error in errors:
            print(f"  - {error}")
        return 1
    
    print("✓ All file references are valid")
    return 0

if __name__ == '__main__':
    sys.exit(validate_copilot_instructions())
