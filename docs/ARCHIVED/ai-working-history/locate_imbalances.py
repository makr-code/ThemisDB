#!/usr/bin/env python3

import os
import re

files_to_check = {
    "gguf_loader.cpp": (-3, "extra closing"),
    "model_downloader.cpp": (-2, "extra closing"),
    "inference_engine_enhanced.cpp": (1, "missing closing"),
    "llama_wrapper.cpp": (2, "missing closing"),
    "llm_model_storage.cpp": (2, "missing closing"),
    "streaming_handler.cpp": (1, "missing closing"),
}

base_path = "/home/runner/work/ThemisDB/ThemisDB/src/llm/"

def find_imbalances(filename, expected_balance):
    fpath = os.path.join(base_path, filename)
    if not os.path.exists(fpath):
        print(f"File not found: {filename}")
        return
    
    with open(fpath, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    
    print(f"\n\n{'='*80}")
    print(f"FILE: {filename} (expected balance: {expected_balance})")
    print(f"{'='*80}")
    
    balance = 0
    for i, line in enumerate(lines, 1):
        # Remove raw strings
        clean = re.sub(r'R"\([^)]*\)"', '', line, flags=re.DOTALL)
        # Remove regular strings
        clean = re.sub(r'"[^"]*"', '', clean)
        # Remove character literals
        clean = re.sub(r"'[^']*'", '', clean)
        # Remove // comments
        clean = re.sub(r'//.*$', '', clean)
        # Remove /* */ comments
        clean = re.sub(r'/\*.*?\*/', '', clean, flags=re.DOTALL)
        
        opens = clean.count('{')
        closes = clean.count('}')
        
        if opens > 0 or closes > 0:
            new_balance = balance + opens - closes
            if opens > 0 or closes > 0:
                print(f"Line {i:4d}: {opens}{closes:+3d} = {balance:+3d} -> {new_balance:+3d}  |  {line.rstrip()[:70]}")
            balance = new_balance
    
    print(f"\nFinal balance: {balance} (expected: {expected_balance})")

for fname, balance in files_to_check.items():
    find_imbalances(fname, balance[0])
