import os
import sys
import yaml

def check_doc_header(file_path):
    with open(file_path, 'r') as file:
        header = file.readline().strip()
        if not header.startswith('# '):
            print(f'Header not found in {file_path}')
            return False
    return True

def main(mode):
    if mode == 'changed-only':
        # Logic to find changed files goes here
        changed_files = ['example1.md', 'example2.md']  # Placeholder
        for file in changed_files:
            check_doc_header(file)
    else:
        for root, _, files in os.walk('.'):  
            for file in files:
                if file.endswith('.md'):
                    check_doc_header(os.path.join(root, file))

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Usage: python3 scripts/doc-header-check.py --mode <mode>')
        sys.exit(1)
    main(sys.argv[1])