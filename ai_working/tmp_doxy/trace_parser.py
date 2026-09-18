import importlib.util, sys
spec = importlib.util.spec_from_file_location('doxy_mod', 'tools/python_tools/doxygen_function_header_writer.py')
mod = importlib.util.module_from_spec(spec)
sys.modules['doxy_mod'] = mod
spec.loader.exec_module(mod)
text = open('ai_working/tmp_doxy/include/parser_probe.h', encoding='utf-8').read()
for line in text.splitlines():
    print('LINE:', line)
    if 'defined' in line or 'declared' in line:
        s = line.strip()
        print('looks', mod.looks_like_function_signature(s))
        print('parse', mod.parse_signature(s))
        print('---')
print('all funcs', mod.find_functions(text.splitlines()))
