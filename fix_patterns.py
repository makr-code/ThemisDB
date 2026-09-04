import re

def fix_file_patterns(filepath, patterns):
    """Fix -Wparentheses patterns in a file"""
    with open(filepath, 'r') as f:
        content = f.read()
    
    for old_pattern, new_pattern in patterns:
        content = content.replace(old_pattern, new_pattern)
    
    with open(filepath, 'w') as f:
        f.write(content)

# Fix replication_manager.cpp patterns
rep_patterns = [
    ('    while (pos <static_cast<int>(json_doc.size()) && (json_doc[pos] == \' \' || json_doc[pos] == \':\')) {',
     '    while ((pos < static_cast<int>(json_doc.size())) && (json_doc[pos] == \' \' || json_doc[pos] == \':\')) {'),
    
    ('            while (vp <static_cast<int>(doc.size()) && (doc[vp] == \' \' || doc[vp] == \':\')) {',
     '            while ((vp < static_cast<int>(doc.size())) && (doc[vp] == \' \' || doc[vp] == \':\')) {'),
    
    ('                while (vp <static_cast<int>(merged.size()) && (merged[vp] == \' \' || merged[vp] == \':\')) {',
     '                while ((vp < static_cast<int>(merged.size())) && (merged[vp] == \' \' || merged[vp] == \':\')) {'),
    
    ('        while (vp <static_cast<int>(json.size()) && (json[vp] == \' \' || json[vp] == \':\')) {',
     '        while ((vp < static_cast<int>(json.size())) && (json[vp] == \' \' || json[vp] == \':\')) {'),
    
    ('        while (vp <static_cast<int>(doc.size()) && (doc[vp] == \' \' || doc[vp] == \':\')) {',
     '        while ((vp < static_cast<int>(doc.size())) && (doc[vp] == \' \' || doc[vp] == \':\')) {'),
    
    ('    while (pos <static_cast<int>(doc.size()) && (doc[pos] == \' \' || doc[pos] == \':\')) {',
     '    while ((pos < static_cast<int>(doc.size())) && (doc[pos] == \' \' || doc[pos] == \':\')) {'),
    
    ('    while (pos <static_cast<int>(doc.size()) && (doc[pos] == \' \' || doc[pos] == \':\')) {',
     '    while ((pos < static_cast<int>(doc.size())) && (doc[pos] == \' \' || doc[pos] == \':\')) {'),
    
    ('                    while (vp <static_cast<int>(obj.size()) && (obj[vp] == \' \' || obj[vp] == \':\')) {',
     '                    while ((vp < static_cast<int>(obj.size())) && (obj[vp] == \' \' || obj[vp] == \':\')) {'),
    
    ('    if (config_.track_origin && (entry.origin_node.empty() || entry.origin_seq == 0)) {',
     '    if ((config_.track_origin) && (entry.origin_node.empty() || entry.origin_seq == 0)) {'),
]

fix_file_patterns('src/replication/replication_manager.cpp', rep_patterns)
print("Fixed replication_manager.cpp")

