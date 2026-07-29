from pathlib import Path
import sys
p=Path('tests/test_hot_reload_api_handler.cpp')
if not p.exists():
    print('file not found', p)
    sys.exit(1)
s=p.read_text(encoding='utf-8')
stack=[]
line=1
col=0
for ch in s:
    col+=1
    if ch=='\n':
        line+=1; col=0
        continue
    if ch=='{':
        stack.append((line,col))
    elif ch=='}':
        if not stack:
            print('Unmatched } at line',line,'col',col)
            sys.exit(0)
        stack.pop()
if stack:
    print('Unmatched { at first occurrence line',stack[0][0],'col',stack[0][1])
else:
    print('Braces balanced')
