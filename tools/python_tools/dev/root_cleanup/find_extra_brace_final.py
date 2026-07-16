#!/usr/bin/env python3
"""Find the extra closing brace by comparing function definitions to closings"""

with open('src/graph/ontology_manager.cpp', 'r') as f:
    lines = f.readlines()

# Count function definitions in anonymous namespace
import re

print("Function definitions in anonymous namespace (52-293):")
print()

balance = 0
func_opens = 0
func_closes = 0

for i in range(51, 293):  # Line 52 to 293
    line = lines[i]
    
    # Check for function definitions (static keyword)
    if re.search(r'\bstatic\b.*\{', line):
        func_opens += 1
        print(f"Line {i+1:3d}: Function START: {line.rstrip()[:70]}")
    
    # Check for top-level closing braces
    if line.strip() == '}' or line.strip().startswith('}'):
        # It's a closing brace
        func_closes += 1

print()
print(f"Function-related braces:")
print(f"  Function definitions (with {{): {func_opens}")
print(f"  Top-level closing braces (only }}: {func_closes}")
print()

# The anonymous namespace opens at 52 and closes at 293
# Between them should be: 1 anonymous ns open + N function opens = N+1 total opens
# And: N function closes + 1 anon ns close = N+1 total closes
# But we found: 52 opens, 53 closes

# Analyze the actual balance by tracking each close
print("Finding where balance goes negative...")
balance = 0
for i in range(51, 293):
    opens = lines[i].count('{')
    closes = lines[i].count('}')
    balance += opens - closes
    
    if closes > opens and balance <= 0:
        print(f"Line {i+1:3d}: balance={balance:3d} | closes > opens | {lines[i].rstrip()[:70]}")

print()
print(f"At line 293: balance should be 1 (for anonymous ns close), actual = {balance + 1}")
