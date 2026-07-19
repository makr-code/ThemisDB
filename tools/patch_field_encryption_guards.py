import os
import re

root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
tests_dir = os.path.join(root, 'tests')
patterns = ['make_shared<FieldEncryption', 'make_unique<FieldEncryption', 'new FieldEncryption', 'FieldEncryption(', 'setFieldEncryption(']

guard_lines = [
    '        std::string edition_err;',
    '        if (!themis::edition::EditionManager::instance().isFeatureAvailable("field_encryption", edition_err)) { GTEST_SKIP() << "Field encryption unavailable: " << edition_err; }',
]

def insert_into_setup(text):
    # find 'void SetUp() override' and the opening brace
    m = re.search(r'void\s+SetUp\s*\(\)\s*override\s*\{', text)
    if not m:
        return None
    start = m.end()  # position after '{'
    # find function body end by simple brace counting
    brace = 1
    i = start
    while i < len(text):
        if text[i] == '{':
            brace += 1
        elif text[i] == '}':
            brace -= 1
            if brace == 0:
                end = i
                break
        i += 1
    else:
        return None
    func_text = text[m.start():end+1]
    # if guard already present or edition_err already declared, skip
    if 'isFeatureAvailable("field_encryption"' in func_text or 'edition_err' in func_text:
        return None
    # insert guard after the opening brace with same indentation as following line
    insert_pos = start
    # Determine indentation (use 8 spaces default)
    indent = '\n'
    # build insertion string
    ins = '\n' + '\n'.join(guard_lines) + '\n'
    new_text = text[:insert_pos] + ins + text[insert_pos:]
    return new_text


modified = []
for dirpath, dirs, files in os.walk(tests_dir):
    for fname in files:
        if not fname.endswith('.cpp'):
            continue
        path = os.path.join(dirpath, fname)
        try:
            with open(path, 'r', encoding='utf-8') as fh:
                s = fh.read()
        except Exception:
            continue
        # quick check for concrete patterns
        if not any(p in s for p in patterns):
            continue
        if 'isFeatureAvailable("field_encryption"' in s or "isFeatureAvailable('field_encryption'" in s:
            continue
        new = insert_into_setup(s)
        if new:
            with open(path, 'w', encoding='utf-8') as fh:
                fh.write(new)
            modified.append(path.replace('\\','/'))

for m in modified:
    print('Patched:', m)

print('PatchedCount=' + str(len(modified)))
