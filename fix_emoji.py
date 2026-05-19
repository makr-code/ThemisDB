#!/usr/bin/env python3
import re

# Read the file
with open('tools/migrate_issues_to_7phase.py', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace all emoji with ASCII equivalents
replacements = {
    '🔄': '[MIGRATE]',
    '⚠️': '[WARN]',
    '📋': '[TASKS]',
    'ℹ️': '[INFO]',
    '💡': '[TIP]',
    '✅': '[OK]',
    '❌': '[ERROR]',
    '🔴': '[CRITICAL]',
    '🟠': '[HIGH]',
    '🟡': '[MEDIUM]',
    '📊': '[STATS]',
    '🚀': '[WORKFLOW]',
    '📐': '[PLANNING]',
    '💻': '[CODE]',
    '🤖': '[AUTO]',
    '👤': '[REVIEW]',
    '📚': '[DOCS]',
    '🎉': '[DONE]',
    '🚨': '[ERROR]',
    '📝': '[NOTE]',
    '☞': '=>',
    '⏭️': '[SKIP]',
}

for emoji, ascii_repr in replacements.items():
    content = content.replace(emoji, ascii_repr)

# Write back
with open('tools/migrate_issues_to_7phase.py', 'w', encoding='utf-8') as f:
    f.write(content)

print("[OK] Replaced all emoji with ASCII equivalents")
