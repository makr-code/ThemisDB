import importlib.util, sys
spec = importlib.util.spec_from_file_location('doxy_mod', r'C:\Projects\ThemisDB\tools\python_tools\doxygen_function_header_writer.py')
mod = importlib.util.module_from_spec(spec)
sys.modules['doxy_mod'] = mod
spec.loader.exec_module(mod)
text = open(r'C:\Projects\ThemisDB\ai_working\tmp_doxy\include\parser_probe.h', encoding='utf-8').read()
funcs = mod.find_functions(text.splitlines())
print('count', len(funcs))
for f in funcs:
    print(f.name, 'has_body=', f.has_body, 'sig=', f.signature)
print('throws', mod.describe_throws(['std::runtime_error']))
print('called', mod.extract_called_functions('if (x < 0) throw std::runtime_error("bad"); return x;', 'defined'))
