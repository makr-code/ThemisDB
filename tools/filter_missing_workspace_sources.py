from pathlib import Path
root = Path(r"C:\Projects\ThemisDB")
miss = root / 'build' / 'missing_cmake_sources.txt'
out = root / 'build' / 'missing_cmake_sources_workspace_only.txt'
if not miss.exists():
    print('Missing master file not found:', miss)
    raise SystemExit(1)

with miss.open('r', encoding='utf-8') as f:
    lines = f.readlines()

out_lines = []
current_block = []
for line in lines:
    if line.strip()=='' and current_block:
        # decide if block belongs to workspace by simple substring checks
        header = current_block[0]
        path = header.strip()
        if ('\\src\\' in path) or ('\\tests\\' in path) or ('llama.cpp' in path) or ('stable-diffusion.cpp' in path) or ('whisper.cpp' in path):
            out_lines.extend(current_block)
            out_lines.append('\n')
        current_block = []
    else:
        current_block.append(line)
# flush
if current_block:
    header = current_block[0]
    path = header.strip()
    if ('\\src\\' in path) or ('\\tests\\' in path) or ('llama.cpp' in path) or ('stable-diffusion.cpp' in path) or ('whisper.cpp' in path):
        out_lines.extend(current_block)
        out_lines.append('\n')

out.parent.mkdir(parents=True, exist_ok=True)
with out.open('w', encoding='utf-8') as f:
    f.writelines(out_lines)

print('Wrote', out, 'with', len(out_lines), 'lines')
