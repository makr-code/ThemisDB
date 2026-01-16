#!/usr/bin/env python3
"""
Thematic Code Counter for ThemisDB
Counts C++ code lines by theme/category for the date 16.01.2026
"""

import os
import subprocess
from pathlib import Path
from collections import defaultdict
from datetime import datetime

def count_lines_in_files(file_list):
    """Count total lines in a list of files."""
    if not file_list:
        return 0
    try:
        result = subprocess.run(
            ['wc', '-l'] + file_list,
            capture_output=True,
            text=True,
            check=True
        )
        # Last line contains total
        lines = result.stdout.strip().split('\n')
        if len(lines) > 1:
            total_line = lines[-1]
            return int(total_line.split()[0])
        elif len(lines) == 1:
            return int(lines[0].split()[0])
    except Exception as e:
        print(f"Error counting lines: {e}")
        return 0
    return 0

def find_cpp_files(directory):
    """Find all C++ files in a directory."""
    cpp_extensions = ['.cpp', '.h', '.hpp', '.cc', '.cxx']
    cpp_files = []
    
    if not os.path.exists(directory):
        return []
    
    for root, dirs, files in os.walk(directory):
        for file in files:
            if any(file.endswith(ext) for ext in cpp_extensions):
                cpp_files.append(os.path.join(root, file))
    
    return cpp_files

def count_by_category(base_dir):
    """Count C++ code lines by category/theme."""
    categories = {}
    
    if not os.path.exists(base_dir):
        return categories
    
    # Get immediate subdirectories
    subdirs = [d for d in os.listdir(base_dir) 
               if os.path.isdir(os.path.join(base_dir, d))]
    
    for subdir in sorted(subdirs):
        subdir_path = os.path.join(base_dir, subdir)
        cpp_files = find_cpp_files(subdir_path)
        if cpp_files:
            line_count = count_lines_in_files(cpp_files)
            file_count = len(cpp_files)
            categories[subdir] = {
                'lines': line_count,
                'files': file_count
            }
    
    # Also count files directly in the base directory
    direct_files = [os.path.join(base_dir, f) 
                    for f in os.listdir(base_dir) 
                    if os.path.isfile(os.path.join(base_dir, f)) 
                    and any(f.endswith(ext) for ext in ['.cpp', '.h', '.hpp', '.cc', '.cxx'])]
    
    if direct_files:
        line_count = count_lines_in_files(direct_files)
        categories['[root]'] = {
            'lines': line_count,
            'files': len(direct_files)
        }
    
    return categories

def count_themis_only(include_dir):
    """Count only Themis-related files in include directory."""
    themis_path = os.path.join(include_dir, 'themis')
    
    if not os.path.exists(themis_path):
        return {}, 0, 0
    
    # Count by subdirectories in themis
    categories = {}
    subdirs = [d for d in os.listdir(themis_path) 
               if os.path.isdir(os.path.join(themis_path, d))]
    
    for subdir in sorted(subdirs):
        subdir_path = os.path.join(themis_path, subdir)
        cpp_files = find_cpp_files(subdir_path)
        if cpp_files:
            line_count = count_lines_in_files(cpp_files)
            categories[subdir] = {
                'lines': line_count,
                'files': len(cpp_files)
            }
    
    # Count files directly in themis directory
    direct_files = [os.path.join(themis_path, f) 
                    for f in os.listdir(themis_path) 
                    if os.path.isfile(os.path.join(themis_path, f)) 
                    and any(f.endswith(ext) for ext in ['.cpp', '.h', '.hpp', '.cc', '.cxx'])]
    
    if direct_files:
        line_count = count_lines_in_files(direct_files)
        categories['[themis-root]'] = {
            'lines': line_count,
            'files': len(direct_files)
        }
    
    # Calculate totals
    total_lines = sum(cat['lines'] for cat in categories.values())
    total_files = sum(cat['files'] for cat in categories.values())
    
    return categories, total_lines, total_files

def count_directory_total(directory, description):
    """Count total C++ code in a directory."""
    cpp_files = find_cpp_files(directory)
    if cpp_files:
        line_count = count_lines_in_files(cpp_files)
        return {
            'description': description,
            'lines': line_count,
            'files': len(cpp_files)
        }
    return {
        'description': description,
        'lines': 0,
        'files': 0
    }

def generate_report(repo_root):
    """Generate comprehensive counting report."""
    report_lines = []
    report_lines.append("# ThemisDB Code Count Report")
    report_lines.append(f"## Date: {datetime.now().strftime('%d.%m.%Y %H:%M:%S')}")
    report_lines.append("")
    report_lines.append("This report provides a thematic count of C++ code in ThemisDB.")
    report_lines.append("")
    
    # Count src by theme
    report_lines.append("## 1. Source Code by Theme (`./src`)")
    report_lines.append("")
    src_categories = count_by_category(os.path.join(repo_root, 'src'))
    
    if src_categories:
        report_lines.append("| Theme | Files | Lines |")
        report_lines.append("|-------|-------|-------|")
        total_src_lines = 0
        total_src_files = 0
        for theme, data in sorted(src_categories.items()):
            report_lines.append(f"| {theme} | {data['files']} | {data['lines']:,} |")
            total_src_lines += data['lines']
            total_src_files += data['files']
        report_lines.append(f"| **TOTAL** | **{total_src_files}** | **{total_src_lines:,}** |")
    else:
        report_lines.append("*No C++ files found*")
    report_lines.append("")
    
    # Count include by theme (all categories)
    report_lines.append("## 2. Headers by Theme (`./include`)")
    report_lines.append("")
    include_categories = count_by_category(os.path.join(repo_root, 'include'))
    
    if include_categories:
        report_lines.append("| Theme | Files | Lines |")
        report_lines.append("|-------|-------|-------|")
        total_include_lines = 0
        total_include_files = 0
        for theme, data in sorted(include_categories.items()):
            report_lines.append(f"| {theme} | {data['files']} | {data['lines']:,} |")
            total_include_lines += data['lines']
            total_include_files += data['files']
        report_lines.append(f"| **TOTAL** | **{total_include_files}** | **{total_include_lines:,}** |")
    else:
        report_lines.append("*No C++ files found*")
    report_lines.append("")
    
    # Count Themis-specific in include
    report_lines.append("## 3. Themis-Specific Headers (`./include/themis`)")
    report_lines.append("")
    themis_categories, themis_lines, themis_files = count_themis_only(os.path.join(repo_root, 'include'))
    
    if themis_categories:
        report_lines.append("| Component | Files | Lines |")
        report_lines.append("|-----------|-------|-------|")
        for component, data in sorted(themis_categories.items()):
            report_lines.append(f"| {component} | {data['files']} | {data['lines']:,} |")
        report_lines.append(f"| **TOTAL** | **{themis_files}** | **{themis_lines:,}** |")
    else:
        report_lines.append("*No Themis-specific files found*")
    report_lines.append("")
    
    # Count tools
    report_lines.append("## 4. Tools (`./tools`)")
    report_lines.append("")
    tools_data = count_directory_total(os.path.join(repo_root, 'tools'), "Development tools")
    report_lines.append(f"- **Files**: {tools_data['files']}")
    report_lines.append(f"- **Lines**: {tools_data['lines']:,}")
    report_lines.append("")
    
    # Count examples
    report_lines.append("## 5. Examples (`./examples`)")
    report_lines.append("")
    examples_data = count_directory_total(os.path.join(repo_root, 'examples'), "Example applications")
    report_lines.append(f"- **Files**: {examples_data['files']}")
    report_lines.append(f"- **Lines**: {examples_data['lines']:,}")
    report_lines.append("")
    
    # Count tests
    report_lines.append("## 6. Tests (`./tests`)")
    report_lines.append("")
    tests_data = count_directory_total(os.path.join(repo_root, 'tests'), "Test suite")
    report_lines.append(f"- **Files**: {tests_data['files']}")
    report_lines.append(f"- **Lines**: {tests_data['lines']:,}")
    report_lines.append("")
    
    # Count benchmarks
    report_lines.append("## 7. Benchmarks (`./benchmarks`)")
    report_lines.append("")
    benchmarks_data = count_directory_total(os.path.join(repo_root, 'benchmarks'), "Performance benchmarks")
    report_lines.append(f"- **Files**: {benchmarks_data['files']}")
    report_lines.append(f"- **Lines**: {benchmarks_data['lines']:,}")
    report_lines.append("")
    
    # Count scripts
    report_lines.append("## 8. Scripts (`./scripts`)")
    report_lines.append("")
    scripts_data = count_directory_total(os.path.join(repo_root, 'scripts'), "Build and deployment scripts")
    report_lines.append(f"- **Files**: {scripts_data['files']}")
    report_lines.append(f"- **Lines**: {scripts_data['lines']:,}")
    report_lines.append("")
    
    # Grand total
    report_lines.append("## 9. Grand Total")
    report_lines.append("")
    grand_total_lines = (
        total_src_lines + 
        total_include_lines + 
        tools_data['lines'] + 
        examples_data['lines'] + 
        tests_data['lines'] + 
        benchmarks_data['lines'] +
        scripts_data['lines']
    )
    grand_total_files = (
        total_src_files + 
        total_include_files + 
        tools_data['files'] + 
        examples_data['files'] + 
        tests_data['files'] + 
        benchmarks_data['files'] +
        scripts_data['files']
    )
    
    report_lines.append("| Category | Files | Lines |")
    report_lines.append("|----------|-------|-------|")
    report_lines.append(f"| Source (`./src`) | {total_src_files} | {total_src_lines:,} |")
    report_lines.append(f"| Headers (`./include`) | {total_include_files} | {total_include_lines:,} |")
    report_lines.append(f"| Tools | {tools_data['files']} | {tools_data['lines']:,} |")
    report_lines.append(f"| Examples | {examples_data['files']} | {examples_data['lines']:,} |")
    report_lines.append(f"| Tests | {tests_data['files']} | {tests_data['lines']:,} |")
    report_lines.append(f"| Benchmarks | {benchmarks_data['files']} | {benchmarks_data['lines']:,} |")
    report_lines.append(f"| Scripts | {scripts_data['files']} | {scripts_data['lines']:,} |")
    report_lines.append(f"| **GRAND TOTAL** | **{grand_total_files}** | **{grand_total_lines:,}** |")
    report_lines.append("")
    
    # Additional statistics
    report_lines.append("## 10. Statistics")
    report_lines.append("")
    if grand_total_files > 0:
        avg_lines_per_file = grand_total_lines / grand_total_files
        report_lines.append(f"- **Average lines per file**: {avg_lines_per_file:.1f}")
    report_lines.append(f"- **Themis-specific files in include**: {themis_files} files, {themis_lines:,} lines")
    report_lines.append("")
    
    report_lines.append("---")
    report_lines.append("*Report generated automatically for issue #581*")
    
    return '\n'.join(report_lines)

def main():
    """Main execution function."""
    repo_root = '/home/runner/work/ThemisDB/ThemisDB'
    
    print("Generating thematic code count report...")
    report = generate_report(repo_root)
    
    # Save report
    report_file = os.path.join(repo_root, 'CODE_COUNT_REPORT_20260116.md')
    with open(report_file, 'w') as f:
        f.write(report)
    
    print(f"Report saved to: {report_file}")
    print("\nReport Preview:")
    print("=" * 80)
    print(report)
    print("=" * 80)

if __name__ == '__main__':
    main()
