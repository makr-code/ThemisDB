import os
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
matches = []
for dirpath, dirs, files in os.walk(os.path.join(root,'tests')):
    for fname in files:
        if not fname.endswith('.cpp'):
            continue
        path = os.path.join(dirpath, fname)
        try:
            with open(path, 'r', encoding='utf-8') as f:
                s = f.read()
        except Exception as e:
            continue
        # patterns indicating concrete usage
        concrete = False
        patterns = ['make_shared<FieldEncryption', 'make_unique<FieldEncryption', 'new FieldEncryption', 'FieldEncryption(', 'setFieldEncryption(']
        for p in patterns:
            if p in s:
                concrete = True
                break
        if not concrete:
            continue
        if 'isFeatureAvailable("field_encryption"' in s or "isFeatureAvailable('field_encryption'" in s:
            continue
        matches.append(path.replace('\\','/'))

for m in matches:
    print(m)

print('FOUND_COUNT=' + str(len(matches)))
