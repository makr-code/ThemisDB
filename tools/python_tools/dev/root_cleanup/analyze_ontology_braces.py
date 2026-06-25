#!/usr/bin/env python3
"""Find the exact location of the extra brace by checking balance at key points"""

with open('src/graph/ontology_manager.cpp', 'r') as f:
    lines = f.readlines()

# Count opening and closing braces, track cumulative balance
balance = 0
issue_found = False

# Lines to check
checkpoints = [
    (52, "Anonymous namespace opens"),
    (293, "Anonymous namespace closes (should be balance=2)"),
    (299, "loadFromJson function"),
    (534, "parseJson function starts"),
    (619, "parseJson function ends"),
    (621, "parseYaml function starts"),
    (700, "parseYaml function ends (should be balance=2)"),
    (702, "namespace graph closes (should be balance=1)"),
    (703, "namespace themis closes (should be balance=0)"),
]

balance = 0
checkpoint_balances = {}

for i, line in enumerate(lines, 1):
    opens = line.count('{')
    closes = line.count('}')
    balance += opens - closes
    
    for cp_line, cp_desc in checkpoints:
        if i == cp_line:
            checkpoint_balances[cp_line] = balance
            print(f"Line {i:4d}: balance = {balance:3d} | {cp_desc}")

print()
print("Analysis:")
print(f"  After anonymous namespace closes (line 293):     expected=2, actual={checkpoint_balances[293]}")
print(f"  After parseJson ends (line 619):                 expected=2, actual={checkpoint_balances[619]}")
print(f"  After parseYaml ends (line 700):                 expected=2, actual={checkpoint_balances[700]}")
print(f"  After namespace graph closes (line 702):         expected=1, actual={checkpoint_balances[702]}")
print(f"  After namespace themis closes (line 703):        expected=0, actual={checkpoint_balances[703]}")

# Find the line with the problem
if checkpoint_balances[293] != 2:
    print(f"\n ERROR: Balance after anonymous namespace is {checkpoint_balances[293]}, not 2!")
    print("  This suggests an extra closing brace in the anonymous namespace or after it.")
    
    # Find the exact line
    balance = 0
    for i, line in enumerate(lines, 1):
        if i <= 293:
            opens = line.count('{')
            closes = line.count('}')
            balance += opens - closes
        if balance < 1 and i > 52:
            print(f"\n  First unbalance at line {i}: balance = {balance}")
            print(f"  Line: {line.rstrip()}")
            break
