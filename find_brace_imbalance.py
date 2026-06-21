#!/usr/bin/env python3
"""Find exact location of brace imbalance in ontology_manager.cpp"""

import re

with open('src/graph/ontology_manager.cpp', 'r') as f:
    content = f.read()

# Count opening and closing braces
opens = content.count('{')
closes = content.count('}')

print(f'Opening braces: {opens}')
print(f'Closing braces: {closes}')
print(f'Balance: {opens - closes}')
print()

# Find where the imbalance starts by scanning line by line
lines = content.split('\n')
balance = 0
for i, line in enumerate(lines, 1):
    # Simple count without filtering (to see raw picture)
    opens_in_line = line.count('{')
    closes_in_line = line.count('}')
    
    balance += opens_in_line - closes_in_line
    
    # Print lines with braces
    if opens_in_line > 0 or closes_in_line > 0:
        print(f'Line {i:4d}: +{opens_in_line} -{closes_in_line} | balance = {balance:3d} | {line[:70]}')
        
        if balance < 0:
            print(f'^^^ IMBALANCE DETECTED at line {i}')
            break

print()
print('Last 5 lines:')
for i in range(max(0, len(lines)-5), len(lines)):
    line = lines[i]
    print(f'Line {i+1:4d}: {line}')
