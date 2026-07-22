from pathlib import Path
root = Path(r"C:\Projects\ThemisDB")
src = root / 'build' / 'missing_workspace_paths_internal.txt'
out = root / 'build' / 'missing_workspace_paths_internal_pruned.txt'
if not src.exists():
    print('Source not found:', src)
    raise SystemExit(1)
lines = [l.strip() for l in src.read_text(encoding='utf-8', errors='ignore').splitlines() if l.strip()]
# patterns to remove (case-insensitive substrings)
patterns = [
    'src/cpp/arrow', 'src\\cpp\\arrow', 'src/cpp\\arrow', 'src/cpp/arrow',
    'src/soil2', 'src\\soil2',
    'src/common/gl_', 'src\\common\\gl_', 'libangle', 'crc32c', 'ducc0', 'vocab\\', 'vocab/',
    'nanobench', 'contrib\\', 'contrib/', 'thirdparty', 'third_party', 'stable-diffusion.cpp', 'llama.cpp', 'ggml', 'whisper.cpp'
]
pruned = []
for l in lines:
    low = l.lower().replace('/', '\\')
    skip = False
    for p in patterns:
        if p.lower().replace('/', '\\') in low:
            skip = True
            break
    if not skip:
        pruned.append(l)
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text('\n'.join(sorted(set(pruned))), encoding='utf-8')
print('Wrote', out, 'entries:', len(set(pruned)))
