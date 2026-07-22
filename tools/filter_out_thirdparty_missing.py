from pathlib import Path
excludes = [
    'vcpkg', 'vcpkg_installed', 'stable-diffusion.cpp', 'llama.cpp', 'ggml', 'whisper.cpp',
    'libwebp', 'libwebm', 'FFmpeg', 'stable-ui', 'thirdparty', 'third_party', 'third_party',
    'tools', '.venv', 'buildtrees', 'vcpkg\buildtrees'
]
root = Path(r"C:\Projects\ThemisDB")
src = root / 'build' / 'missing_workspace_paths_relative.txt'
out = root / 'build' / 'missing_workspace_paths_internal.txt'
if not src.exists():
    print('Source not found:', src)
    raise SystemExit(1)
lines = src.read_text(encoding='utf-8', errors='ignore').splitlines()
kept = []
for L in lines:
    s = L.strip()
    if not s:
        continue
    low = s.replace('/', '\\').lower()
    skip = False
    for ex in excludes:
        exn = ex.replace('/', '\\').lower()
        if low.startswith(exn + '\\') or low == exn or ('\\' + exn + '\\') in ('\\' + low) or low.startswith(exn):
            skip = True
            break
    if not skip:
        kept.append(s)
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text('\n'.join(sorted(set(kept))), encoding='utf-8')
print('Wrote', out, 'entries:', len(set(kept)))
