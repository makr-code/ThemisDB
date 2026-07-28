#!/usr/bin/env python3
import re
import pathlib

root = pathlib.Path('tests')
pattern = re.compile(r'namespace\s+([A-Za-z0-9_:]+)\s*\{')

files_changed = []
for p in root.rglob('*.cpp'):
    text = p.read_text(encoding='utf-8')
    matches = list(pattern.finditer(text))
    if not matches:
        continue
    new_text = text
    modified = False
    for m in reversed(matches):
        ns = m.group(1).strip()
        if '::' not in ns:
            continue
        parts = ns.split('::')
        # Build replacement
        open_repl = ''.join(f'namespace {part} {{ ' for part in parts)
        # Replace the specific match
        start, end = m.span()
        new_text = new_text[:start] + open_repl + new_text[end:]
        modified = True
        # Now look for closing comment exact match
        closing_pattern = re.compile(r"\n\s*}\s*//\s*namespace\s+" + re.escape(ns) + r"\s*\n")
        mclose = closing_pattern.search(new_text)
        if mclose:
            close_repl = '\n' + ('} ' * len(parts)).rstrip() + f' // namespace {ns}\n'
            new_text = new_text[:mclose.start()] + close_repl + new_text[mclose.end():]
        else:
            # try alternative: lines that end with comment containing the ns
            alt_pattern = re.compile(r"\n\s*}\s*//.*" + re.escape(parts[-1]) + r".*\n")
            malt = alt_pattern.search(new_text)
            if malt:
                # conservative: prepend extra braces before this closing brace
                # find the position of the brace
                brace_pos = new_text.rfind('\n}', 0, malt.end())
                if brace_pos != -1:
                    # insert additional closing braces before this '}'
                    insert_pos = brace_pos
                    extra = ('}\n') * (len(parts)-1)
                    new_text = new_text[:insert_pos] + extra + new_text[insert_pos:]
    if modified:
        p.write_text(new_text, encoding='utf-8')
        files_changed.append(str(p))

print('Modified files:')
for f in files_changed:
    print(f)
