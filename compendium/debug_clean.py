from build_pdf_final import clean_md
from pathlib import Path
text = Path('chapter_06_graph.md').read_text(encoding='utf-8')
clean_md(text)
