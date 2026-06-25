#!/usr/bin/env python3
"""Count braces while ignoring comments and strings"""

def count_braces_ignoring_comments(filename):
    """Count { and } while ignoring // and /* */ comments and strings"""
    
    with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    opens = 0
    closes = 0
    i = 0
    
    while i < len(content):
        # Check for line comments //
        if i < len(content) - 1 and content[i:i+2] == '//':
            # Skip to end of line
            while i < len(content) and content[i] not in '\n\r':
                i += 1
            i += 1
            continue
        
        # Check for block comments /* */
        if i < len(content) - 1 and content[i:i+2] == '/*':
            # Skip to */
            i += 2
            while i < len(content) - 1:
                if content[i:i+2] == '*/':
                    i += 2
                    break
                i += 1
            continue
        
        # Check for strings with "
        if content[i] == '"':
            i += 1
            while i < len(content):
                if content[i] == '\\':
                    i += 2  # Skip escaped char
                    continue
                if content[i] == '"':
                    i += 1
                    break
                i += 1
            continue
        
        # Check for strings with '
        if content[i] == "'":
            i += 1
            while i < len(content):
                if content[i] == '\\':
                    i += 2  # Skip escaped char
                    continue
                if content[i] == "'":
                    i += 1
                    break
                i += 1
            continue
        
        # Count braces
        if content[i] == '{':
            opens += 1
        elif content[i] == '}':
            closes += 1
        
        i += 1
    
    return opens, closes

# Test on ontology_manager.cpp
fname = "src/graph/ontology_manager.cpp"
opens, closes = count_braces_ignoring_comments(fname)
balance = opens - closes

print(f"File: {fname}")
print(f"Opens:  {opens}")
print(f"Closes: {closes}")
print(f"Balance: {balance:+d}")
print()

if balance == 0:
    print("✅ File is balanced (ignoring comments/strings)")
else:
    print(f"❌ File is unbalanced - {balance} extra {'closing' if balance < 0 else 'opening'} braces")
