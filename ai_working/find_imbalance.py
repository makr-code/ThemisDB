#!/usr/bin/env python3

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

# Find where the balance becomes significantly negative
open_count = 0
close_count = 0
min_balance = 0
min_line = 0

for i, line in enumerate(lines, 1):
    for char in line:
        if char == '{':
            open_count += 1
        elif char == '}':
            close_count += 1
    
    balance = open_count - close_count
    if balance < min_balance:
        min_balance = balance
        min_line = i

# Show the transition area where balance goes negative
print("Checking for function definitions that might be missing braces...")
print(f"\nMinimum balance: {min_balance} at line {min_line}")
print(f"Final balance: {open_count - close_count}")

# Check for function definitions without bodies
in_func = False
func_name = ""
for i, line in enumerate(lines, 1):
    # Look for likely function signatures (lines ending with { or missing one)
    if '::' in line and '(' in line and ')' in line:
        # Possible function definition
        if '{' not in line and i < len(lines):
            next_line = lines[i] if i < len(lines) else ""
            if '{' not in next_line and 'namespace' not in line:
                print(f"L{i}: Potential missing brace after: {line.rstrip()[:100]}")
