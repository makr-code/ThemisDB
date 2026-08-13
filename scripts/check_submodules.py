#!/usr/bin/env python3
import re, subprocess, os
paths=[]
with open('.gitmodules', 'r', encoding='utf-8') as f:
    for line in f:
        m=re.match(r'\s*path\s*=\s*(\S+)', line)
        if m:
            paths.append(m.group(1))
for p in paths:
    print('---', p, '---')
    if os.path.isdir(p):
        def run(cmd):
            try:
                out=subprocess.check_output(cmd, shell=True, cwd=p, stderr=subprocess.STDOUT, text=True)
                return out.strip()
            except subprocess.CalledProcessError as e:
                return e.output.strip()
        commit = run('git rev-parse --short HEAD') or 'NO_COMMIT'
        print('commit:'+commit)
        print('porcelain:')
        print(run('git status --porcelain'))
        print('branch:'+run('git rev-parse --abbrev-ref HEAD'))
        print('remote:')
        print(run('git remote -v'))
        ls = run(f'git ls-remote origin {commit}')
        print('commit-on-origin: ' + ('yes' if commit in ls else 'NO'))
    else:
        print('MISSING PATH')
