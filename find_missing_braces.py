#!/usr/bin/env python3

import os
import re

files_to_analyze = {
    "gguf_loader.cpp": -3,
    "model_downloader.cpp": -2,
    "inference_engine_enhanced.cpp": 1,
    "llama_wrapper.cpp": 2,
    "llm_model_storage.cpp": 2,
    "streaming_handler.cpp": 1,
}

base_path = "/home/runner/work/ThemisDB/ThemisDB/src/llm/"

def find_location(filename, expected_balance):
    fpath = os.path.join(base_path, filename)
    with open(fpath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Clean properly
    clean = re.sub(r'R"\([^)]*\)"', '', content, flags=re.DOTALL)
    clean = re.sub(r'"[^"]*"', '', clean)
    clean = re.sub(r"'[^']*'", '', clean)
    clean = re.sub(r'//.*$', '', clean, flags=re.MULTILINE)
    clean = re.sub(r'/\*.*?\*/', '', clean, flags=re.DOTALL)
    
    balance = 0
    for i, line in enumerate(clean.split('\n'), 1):
        for char in line:
            if char == '{':
                balance += 1
            elif char == '}':
                balance -= 1
    
    print(f"\n{filename} (expected: {expected_balance})")
    print(f"  Final balance: {balance}")
    
    if balance != 0:
        # Try to find the problem by checking around major functions
        lines = content.split('\n')
        print(f"  Total lines: {len(lines)}")
        
        # Find functions with their line numbers
        for i, line in enumerate(lines[-30:], len(lines)-30):
            if 'namespace' in line or '}' in line and i > len(lines) - 10:
                print(f"  Line {i}: {line}")

for fname, balance in files_to_analyze.items():
    find_location(fname, balance)
