import importlib.util, sys
spec = importlib.util.spec_from_file_location('doxy_mod', 'tools/python_tools/doxygen_function_header_writer.py')
mod = importlib.util.module_from_spec(spec)
sys.modules['doxy_mod'] = mod
spec.loader.exec_module(mod)

line = 'int defined(int x) { if (x < 0) throw std::runtime_error("bad"); return x; }'
print('looks', mod.looks_like_function_signature(line))
print('parse', mod.parse_signature(line))
print('comments stripped', mod.strip_strings_and_comments(line))

text = open('ai_working/tmp_doxy/include/parser_probe.h', encoding='utf-8').read()
for ln in text.splitlines():
    if 'defined' in ln or 'declared' in ln:
        print('--- LINE ---', ln)
        print('looks', mod.looks_like_function_signature(ln.strip()))
        print('parse', mod.parse_signature(ln.strip()))
