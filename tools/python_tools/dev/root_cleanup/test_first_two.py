#!/usr/bin/env python3
"""Test unity-style inclusion of first two files"""

files_content = []
files = [
    "src/acceleration/ai_hardware_dispatcher.cpp",
    "src/index/graph_auto_buffer.cpp",
]

import os
os.chdir(r"c:\Projects\ThemisDB")

cumulative = ""
for i, fname in enumerate(files, 1):
    with open(fname, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    cumulative += content
    
    # Check balance
    ops = cumulative.count('{')
    clos = cumulative.count('}')
    balance = ops - clos
    
    print(f"After file {i} ({fname}):")
    print(f"  {i}. Cumulative Opens={ops} Closes={clos} Balance={balance:+d}")
