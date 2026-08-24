#!/usr/bin/env python3

import os
import re

files_to_fix = [
    ("gguf_loader.cpp", -3),
    ("model_downloader.cpp", -2),
    ("inference_engine_enhanced.cpp", 1),
    ("llama_wrapper.cpp", 2),
    ("llm_model_storage.cpp", 2),
    ("streaming_handler.cpp", 1),
]

base_path = "/home/runner/work/ThemisDB/ThemisDB/src/llm/"

def analyze_file_structure(filename):
    """Analyze file structure to find where to add/remove braces"""
    fpath = os.path.join(base_path, filename)
    with open(fpath, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    
    # Find namespace closing lines
    namespace_closes = []
    for i in range(len(lines)-1, -1, -1):
        if re.match(r'^\s*}\s*//\s*namespace', lines[i]):
            namespace_closes.append((i, lines[i]))
        if i < len(lines) - 5 and '} // namespace' not in lines[i]:
            break
    
    print(f"\n{filename}")
    print(f"Last 10 lines:")
    for i in range(max(0, len(lines)-10), len(lines)):
        print(f"  {i+1}: {lines[i].rstrip()}")
    
    return lines

for fname, balance in files_to_fix:
    analyze_file_structure(fname)
