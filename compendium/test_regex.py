import re, pathlib
pattern = re.compile(r'```[ \t]*mermaid\r?\n(.*?)```[ \t]*', re.DOTALL)
for f in ['chapter_01_introduction.md','chapter_06_graph.md','chapter_29_analytics_process_mining.md']:
    text = pathlib.Path(f).read_text(encoding='utf-8')
    matches = pattern.findall(text)
    print(f'{f}: {len(matches)} matches')
