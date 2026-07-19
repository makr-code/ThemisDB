import os
import subprocess
import json

root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
tests_dir = os.path.join(root, 'tests')
patterns = ['make_shared<FieldEncryption', 'make_unique<FieldEncryption', 'new FieldEncryption', 'FieldEncryption(', 'setFieldEncryption(']
found = []
for dirpath, dirs, files in os.walk(tests_dir):
    for f in files:
        if not f.endswith('.cpp'):
            continue
        path = os.path.join(dirpath, f)
        try:
            with open(path, 'r', encoding='utf-8') as fh:
                s = fh.read()
        except:
            continue
        if any(p in s for p in patterns):
            rel = os.path.relpath(path, tests_dir).replace('\\','/')
            found.append(rel)

# Build candidate target names
candidates = set()
for rel in found:
    stem = os.path.splitext(os.path.basename(rel))[0]
    parts = rel.split('/')
    if len(parts) == 1:
        candidates.add(f'test_{stem}_focused')
    else:
        module = parts[0]
        # common module pattern
        candidates.add(f'module_{module}_{stem}_focused')
        candidates.add(f'test_{stem}_focused')
        candidates.add(f'module_{module}_test_{stem}_focused')

print('Found test sources:')
for f in found:
    print(' -', f)
print('\nCandidate targets:')
for t in sorted(candidates):
    print(' -', t)

# Determine build binaryDir from CMakePresets.json (if available)
preset_build_dir = None
presets_path = os.path.join(root, 'CMakePresets.json')
if os.path.exists(presets_path):
    try:
        with open(presets_path, 'r', encoding='utf-8') as fh:
            presets = json.load(fh)
        # find build preset named 'windows-release'
        bp = next((b for b in presets.get('buildPresets', []) if b.get('name') == 'windows-release'), None)
        if bp:
            cfg_name = bp.get('configurePreset')
            cfg = next((c for c in presets.get('configurePresets', []) if c.get('name') == cfg_name), None)
            if cfg and 'binaryDir' in cfg:
                preset_build_dir = cfg['binaryDir'].replace('${sourceDir}', root)
    except Exception:
        preset_build_dir = None

# If we have a build.ninja, read it to validate targets to avoid unknown-target builds
build_ninja_path = None
if preset_build_dir:
    potential = os.path.abspath(preset_build_dir)
    ninja_path = os.path.join(potential, 'build.ninja')
    if os.path.exists(ninja_path):
        build_ninja_path = ninja_path

valid_targets = []
if build_ninja_path:
    try:
        with open(build_ninja_path, 'r', encoding='utf-8') as fh:
            ninja_text = fh.read()
    except Exception:
        ninja_text = ''
    for t in sorted(candidates):
        # check if exe name appears in build.ninja (common pattern for test executables)
        exe_variants = [f'{t}.exe', f'bin_out/{t}.exe', f'bin_out\\{t}.exe', t + '.vcxproj']
        if any(v in ninja_text for v in exe_variants):
            valid_targets.append(t)
        else:
            # also accept direct target names if they appear (fallback)
            if '\n' + t + ':' in ninja_text:
                valid_targets.append(t)
else:
    # no build.ninja available — fall back to attempting builds (existing behavior)
    valid_targets = sorted(candidates)

print('\nValidated build targets:')
for t in valid_targets:
    print(' -', t)

# Build each validated target individually (serial)
for t in valid_targets:
    print('\n--- Building target:', t)
    cmd = ['cmake', '--build', '--preset', 'windows-release', '--target', t, '--parallel', '1']
    try:
        subprocess.run(cmd, cwd=root, check=True)
    except subprocess.CalledProcessError as e:
        print('Build failed for target', t, 'exitcode', e.returncode)
        continue

print('\nDone')
