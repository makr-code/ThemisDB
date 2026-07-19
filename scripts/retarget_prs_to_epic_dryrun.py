#!/usr/bin/env python3
import json, subprocess, re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
artifacts = ROOT / 'artifacts'
artifacts.mkdir(exist_ok=True)
relations_path = artifacts / 'epic_relations.json'

def run(cmd):
    # capture bytes and decode utf-8 with replacement to avoid encoding errors on Windows
    out = subprocess.check_output(cmd, shell=False)
    return out.decode('utf-8', errors='replace')

def reconstruct_relations():
    print('Reconstructing relations from GitHub issues...')
    out = run(['gh','issue','list','--state','all','--limit','1000','--json','number,title,body,url'])
    issues = json.loads(out)
    rels = []
    rx = re.compile(r'#(\d{2,6})')
    for iss in issues:
        title = iss.get('title') or ''
        body = iss.get('body') or ''
        for m in rx.findall(title):
            rels.append({'parent': int(m), 'child': iss['number'], 'child_url': iss['url']})
        for m in rx.findall(body):
            rels.append({'parent': int(m), 'child': iss['number'], 'child_url': iss['url']})
    with relations_path.open('w', encoding='utf8') as f:
        json.dump(rels, f, indent=2)
    print(f'Reconstructed {len(rels)} relations -> {relations_path}')
    return rels

def load_relations():
    if not relations_path.exists():
        return reconstruct_relations()
    return json.loads(relations_path.read_text(encoding='utf8'))

def main():
    relations = load_relations()
    parents = sorted({r['parent'] for r in relations})
    pr_out = run(['gh','pr','list','--state','open','--limit','1000','--json','number,title,body,url,headRefName,baseRefName'])
    prs = json.loads(pr_out)
    report = []
    retargetable = 0
    nobranch = 0
    for pr in prs:
        text = (pr.get('title') or '') + '\n' + (pr.get('body') or '')
        matched = set()
        for p in parents:
            if re.search(rf'#{p}\b', text):
                matched.add(p)
        if not matched:
            continue
        for parent in matched:
            entry = {
                'pr_number': pr['number'], 'pr_url': pr['url'], 'head': pr.get('headRefName'), 'base': pr.get('baseRefName'), 'parent': parent,
                'candidate_branch': None, 'suggestion': None
            }
            try:
                branches = run(['git','branch','-r','--list',f'origin/*{parent}*']).splitlines()
                branches = [b.strip() for b in branches if b.strip()]
            except subprocess.CalledProcessError:
                branches = []
            if branches:
                candidate = branches[0].removeprefix('origin/')
                entry['candidate_branch'] = candidate
                entry['suggestion'] = f'would retarget to {candidate}'
                retargetable += 1
            else:
                entry['suggestion'] = 'no matching epic branch found'
                nobranch += 1
            report.append(entry)

    out_path = artifacts / 'epic_retarget_dryrun_report.json'
    out_path.write_text(json.dumps(report, indent=2), encoding='utf8')
    print(f'Wrote dry-run report to {out_path}. Entries: {len(report)} Retargetable: {retargetable} NoBranch: {nobranch}')

if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as e:
        print('Command failed:', e, file=sys.stderr)
        sys.exit(1)
