#!/usr/bin/env python3

import sys

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

# Count braces in sections
open_count = 0
close_count = 0
issues = []

for i, line in enumerate(lines, 1):
    for char in line:
        if char == '{':
            open_count += 1
        elif char == '}':
            close_count += 1
    
    # Print lines with braces and current balance
    if '{' in line or '}' in line:
        balance = open_count - close_count
        if i <= 100 or i >= len(lines) - 50:
            print(f"L{i:4d} [{balance:+3d}] {line.rstrip()}")

print(f"\nTotal opens: {open_count}, Total closes: {close_count}, Balance: {open_count - close_count}")
