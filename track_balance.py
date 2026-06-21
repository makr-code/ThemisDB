#!/usr/bin/env python3
"""Find the exact line where balance first goes negative"""

with open('src/graph/ontology_manager.cpp', 'r') as f:
    lines = f.readlines()

# Track cumulative balance, starting fresh at line 52 (anon namespace open)
balance = 1  # Start with 1 for the anonymous namespace opening at line 52

print("Tracking balance from line 52 to 293:")
print()

for i in range(51, 293):  # Line 52 to 293
    line = lines[i]
    opens = line.count('{')
    closes = line.count('}')
    prev_balance = balance
    balance += opens - closes
    
    # Show every line where balance changes, especially negatives
    if opens != 0 or closes != 0:
        if balance < 0 or (closes > opens):
            marker = " <-- PROBLEM!" if balance < 0 else ""
            print(f"Line {i+1:3d}: opens={opens} closes={closes} | prev={prev_balance:2d} -> new={balance:2d}{marker} | {line.rstrip()[:65]}")

print()
print(f"Final balance at line 293: {balance}")
print(f"Expected: 0 (anonymous namespace closes)")
print()
if balance != 0:
    print(f"ERROR: Balance is {balance}, should be 0. Extra closing brace(s) detected.")
