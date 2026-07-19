import os
import re
import json
import subprocess

root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
tests_dir = os.path.join(root, 'tests')
presets_path = os.path.join(root, 'CMakePresets.json')

def load_binary_dir():
    if not os.path.exists(presets_path):
        return None
    with open(presets_path, 'r', encoding='utf-8') as fh:
        presets = json.load(fh)
    bp = next((b for b in presets.get('buildPresets', []) if b.get('name') == 'windows-release'), None)
    if not bp:
        return None
    cfg_name = bp.get('configurePreset')
    cfg = next((c for c in presets.get('configurePresets', []) if c.get('name') == cfg_name), None)
    if cfg and 'binaryDir' in cfg:
        return cfg['binaryDir'].replace('${sourceDir}', root)
    return None

binary_dir = load_binary_dir()
build_ninja = None
if binary_dir:
    ninja = os.path.join(binary_dir, 'build.ninja')
    if os.path.exists(ninja):
        build_ninja = ninja

print('binary_dir:', binary_dir)
print('build_ninja:', build_ninja)

# Collect candidate targets from tests tree
candidates = set()
for dirpath, dirs, files in os.walk(tests_dir):
    for f in files:
        if not f.endswith('.cpp'):
            continue
        rel = os.path.relpath(os.path.join(dirpath, f), tests_dir).replace('\\','/')
        stem = os.path.splitext(os.path.basename(rel))[0]
        parts = rel.split('/')
        if len(parts) == 1:
            candidates.add(f'test_{stem}_focused')
        else:
            module = parts[0]
            candidates.add(f'module_{module}_{stem}_focused')
            candidates.add(f'test_{stem}_focused')
            candidates.add(f'module_{module}_test_{stem}_focused')

print('Candidate count:', len(candidates))

# If we have build.ninja, validate which targets correspond to executables in ninja
valid_targets = []
ninja_text = ''
if build_ninja:
    with open(build_ninja, 'r', encoding='utf-8', errors='ignore') as fh:
        ninja_text = fh.read()
    for t in sorted(candidates):
        exe_variants = [f'bin_out/{t}.exe', f'bin_out\\{t}.exe', f'{t}.exe']
        if any(v in ninja_text for v in exe_variants) or ('\n' + t + ':') in ninja_text:
            valid_targets.append(t)
else:
    # fallback: try to build all candidates
    valid_targets = sorted(candidates)

print('Validated targets to build:', len(valid_targets))

failures = {}
for t in valid_targets:
    print('\n--- Building target:', t)
    cmd = ['cmake', '--build', '--preset', 'windows-release', '--target', t, '--parallel', '1']
    try:
        subprocess.run(cmd, cwd=root, check=True)
    except subprocess.CalledProcessError as e:
        print('Build failed for target', t, 'exitcode', e.returncode)
        failures[t] = e.returncode

print('\nBuild sweep complete. Success:', len(valid_targets)-len(failures), 'Failed:', len(failures))
if failures:
    print('Failures:')
    for k,v in failures.items():
        print(' -', k, v)
