from pathlib import Path

template_path = Path('c:\\Projects\\ThemisDB\\.github\\ISSUE_TEMPLATE\\module_task.md')
content = template_path.read_text(encoding='utf-8')
lines = content.splitlines()

print(f'Total lines: {len(lines)}')
print(f'First line: {repr(lines[0])}')
print(f'Has IMPLEMENTATION_TASKS before processing: {"{{{{IMPLEMENTATION_TASKS}}}}" in content}')

# Check for frontmatter
if lines and lines[0].strip() == '---':
    print('Found frontmatter start')
    for idx in range(1, len(lines)):
        if lines[idx].strip() == '---':
            print(f'Found frontmatter end at line {idx}')
            result = '\n'.join(lines[idx + 1:]).strip()
            print(f'Result length: {len(result)}')
            print(f'Has IMPLEMENTATION_TASKS after processing: {"{{{{IMPLEMENTATION_TASKS}}}}" in result}')
            
            # Show the What needs to be done section
            for i, line in enumerate(result.split('\n')):
                if '## What needs to be done' in line:
                    print(f'\nWhat needs to be done section (lines {i}-{i+2}):')
                    for j in range(max(0, i-1), min(len(result.split('\n')), i+3)):
                        print(f"  {result.split('\n')[j]}")
                    break
            break
