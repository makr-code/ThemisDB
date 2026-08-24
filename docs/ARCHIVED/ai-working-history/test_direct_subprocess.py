#!/usr/bin/env python3
import subprocess
import sys
import os
import threading
import time

env = os.environ.copy()
env['PYTHONPATH'] = 'tools'

print("=== DIRECT gap_scanner_v3.py SUBPROCESS TEST ===")
print(f"PYTHONPATH: {env.get('PYTHONPATH')}")
print(f"CWD: {os.getcwd()}\n")

process = subprocess.Popen(
    [sys.executable, 'tools/gap_scanner_v3.py'],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    env=env
)

# Read first 2 seconds of output
start_time = time.time()
output_lines = []

try:
    while time.time() - start_time < 2:
        line = process.stdout.readline()
        if line:
            output_lines.append(line.rstrip())
            if len(output_lines) > 25:  # Limit to 25 lines
                break
        time.sleep(0.05)
except:
    pass

print("FIRST OUTPUT:")
for line in output_lines[:25]:
    print(line)

# Check for key strings
print("\n=== KEYWORD SEARCH ===")
full_text = '\n'.join(output_lines)
if '[DEBUG]' in full_text:
    print("✅ [DEBUG] messages FOUND")
    for line in output_lines:
        if '[DEBUG]' in line:
            print(f"  {line}")
else:
    print("❌ [DEBUG] messages NOT FOUND")

if 'WAVE5' in full_text or 'WAVE 5' in full_text:
    print("✅ WAVE 5 mentions FOUND")
else:
    print("❌ WAVE 5 NOT mentioned")

# Kill process
try:
    process.kill()
except:
    pass

print(f"\nExit code: {process.returncode}")
