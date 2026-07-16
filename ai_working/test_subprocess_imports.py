#!/usr/bin/env python3
import subprocess
import sys

print("Testing gap_scanner_v3.py import status...")
result = subprocess.run(
    [sys.executable, 'tools/gap_scanner_v3.py', '--help'],
    capture_output=True,
    text=True,
    cwd='c:/Projects/ThemisDB'
)

print("STDOUT:")
for line in result.stdout.split('\n')[:20]:
    print(line)

print("\nSTDERR:")
for line in result.stderr.split('\n')[:20]:
    if line.strip():
        print(line)

print(f"\nExit code: {result.returncode}")
