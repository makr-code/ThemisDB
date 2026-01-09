#!/bin/bash
# Fix Mermaid syntax errors in Markdown files

# Ersetze problematische Unicode-Zeichen in allen chapter_*.md Dateien
for file in chapter_*.md; do
    echo "Processing: $file"
    
    # Ersetze Sonderzeichen
    sed -i 's/✓/[OK]/g' "$file"
    sed -i 's/✅/[OK]/g' "$file"
    sed -i 's/❌/[ERROR]/g' "$file"
    sed -i 's/→/->/g' "$file"
    sed -i 's/←/<-/g' "$file"
    sed -i 's/%/ percent /g' "$file"
    sed -i "s/'//g" "$file"  # Remove single quotes
    sed -i 's/–/-/g' "$file"  # Em-dash to hyphen
    sed -i 's/–/-/g' "$file"  # En-dash to hyphen
    
done

echo "✅ Fixed all Mermaid syntax errors!"
