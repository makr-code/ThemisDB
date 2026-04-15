"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            archive_pipeline.py                                ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     262                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Example: External Archive Processing Pipeline for ThemisDB

This script demonstrates the recommended architecture for archive ingestion:
1. Extract archive externally (Python, not ThemisDB)
2. Process each file (text extraction, embedding generation)
3. Create graph relationships
4. Import structured data to ThemisDB via importContent()

Usage:
    python3 examples/archive_pipeline.py documents.zip
"""

import sys
import zipfile
import os
import json
import hashlib
import mimetypes
from pathlib import Path
from typing import List, Dict, Any
import tempfile
import shutil
import requests

# Configuration
THEMIS_BASE_URL = os.environ.get('THEMIS_URL', 'http://localhost:8080')
THEMIS_API_KEY = os.environ.get('THEMIS_API_KEY', '')  # Optional

def extract_archive(archive_path: str, extract_dir: str) -> List[str]:
    """Extract archive to temporary directory"""
    print(f"Extracting {archive_path}...")
    
    extracted_files = []
    with zipfile.ZipFile(archive_path, 'r') as zf:
        for member in zf.namelist():
            # Security: prevent path traversal
            if '..' in member or member.startswith('/'):
                print(f"Skipping suspicious path: {member}")
                continue
            
            zf.extract(member, extract_dir)
            extracted_path = os.path.join(extract_dir, member)
            
            # Only track actual files, not directories
            if os.path.isfile(extracted_path):
                extracted_files.append(extracted_path)
    
    print(f"Extracted {len(extracted_files)} files")
    return extracted_files

def compute_hash(content: bytes) -> str:
    """Compute SHA-256 hash"""
    return hashlib.sha256(content).hexdigest()

def detect_mime_type(file_path: str) -> str:
    """Detect MIME type from file"""
    mime, _ = mimetypes.guess_type(file_path)
    return mime or 'application/octet-stream'

def process_file(file_path: str, relative_path: str) -> Dict[str, Any]:
    """Process a single file - extract text, generate metadata"""
    print(f"Processing: {relative_path}")
    
    with open(file_path, 'rb') as f:
        content = f.read()
    
    mime_type = detect_mime_type(file_path)
    
    # Determine category based on MIME type
    category = 'BINARY'
    if mime_type.startswith('text/'):
        category = 'TEXT'
    elif mime_type.startswith('image/'):
        category = 'IMAGE'
    elif mime_type in ['application/pdf', 'application/msword']:
        category = 'TEXT'
    
    # Extract text (simplified - in production use proper extractors)
    text_content = ""
    if category == 'TEXT' and mime_type.startswith('text/'):
        try:
            text_content = content.decode('utf-8', errors='ignore')
        except:
            pass
    
    # Generate simple chunks (in production: use proper chunking strategy)
    chunks = []
    if text_content and len(text_content) > 0:
        chunk_size = 1000
        for i in range(0, len(text_content), chunk_size):
            chunk_text = text_content[i:i+chunk_size]
            chunks.append({
                'text': chunk_text,
                'seq_num': len(chunks),
                'chunk_type': 'text',
                # In production: generate real embeddings here
                # embedding = generate_embedding(chunk_text)
                'metadata': {
                    'start_offset': i,
                    'end_offset': min(i + chunk_size, len(text_content))
                }
            })
    
    return {
        'content': {
            'mime_type': mime_type,
            'category': category,
            'original_filename': os.path.basename(file_path),
            'virtual_path': relative_path,
            'size_bytes': len(content),
            'hash_sha256': compute_hash(content),
            'extracted_metadata': {
                'char_count': len(text_content) if text_content else 0
            }
        },
        'chunks': chunks,
        'blob': content
    }

def create_archive_metadata(archive_path: str, member_ids: List[str]) -> Dict[str, Any]:
    """Create metadata for the archive itself"""
    with open(archive_path, 'rb') as f:
        content = f.read()
    
    return {
        'content': {
            'mime_type': 'application/zip',
            'category': 'ARCHIVE',
            'original_filename': os.path.basename(archive_path),
            'size_bytes': len(content),
            'hash_sha256': compute_hash(content),
            'extracted_metadata': {
                'member_count': len(member_ids),
                'extraction_strategy': 'external_pipeline'
            },
            'child_ids': member_ids
        },
        'edges': [
            {
                'from_type': 'content',
                'to_type': 'content', 
                'edge_type': 'CONTAINS',
                'to': member_id,
                'metadata': {'extraction_order': i}
            }
            for i, member_id in enumerate(member_ids)
        ]
    }

def import_to_themis(data: Dict[str, Any], blob: bytes = None) -> str:
    """Import structured data to ThemisDB"""
    headers = {
        'Content-Type': 'application/json'
    }
    if THEMIS_API_KEY:
        headers['Authorization'] = f'Bearer {THEMIS_API_KEY}'
    
    payload = data.copy()
    if blob:
        import base64
        payload['blob_base64'] = base64.b64encode(blob).decode('utf-8')
    
    response = requests.post(
        f'{THEMIS_BASE_URL}/content/import',
        headers=headers,
        json=payload
    )
    
    if response.status_code != 200:
        raise Exception(f"Import failed: {response.text}")
    
    result = response.json()
    return result.get('content_id', '')

def process_archive_pipeline(archive_path: str):
    """Main pipeline: Extract → Process → Import"""
    print(f"\n{'='*60}")
    print(f"External Archive Processing Pipeline")
    print(f"Archive: {archive_path}")
    print(f"Target: {THEMIS_BASE_URL}")
    print(f"{'='*60}\n")
    
    # Create temporary directory
    temp_dir = tempfile.mkdtemp(prefix='themis_archive_')
    
    try:
        # Step 1: Extract (external)
        extracted_files = extract_archive(archive_path, temp_dir)
        
        # Step 2: Process each file (external)
        member_ids = []
        for file_path in extracted_files:
            relative_path = os.path.relpath(file_path, temp_dir)
            
            file_data = process_file(file_path, relative_path)
            
            # Step 3: Import to ThemisDB (structured data)
            try:
                content_id = import_to_themis(file_data, file_data['blob'])
                member_ids.append(content_id)
                print(f"✓ Imported: {relative_path} → {content_id}")
            except Exception as e:
                print(f"✗ Failed to import {relative_path}: {e}")
        
        # Step 4: Create archive metadata with graph edges
        archive_data = create_archive_metadata(archive_path, member_ids)
        archive_id = import_to_themis(archive_data, open(archive_path, 'rb').read())
        
        print(f"\n{'='*60}")
        print(f"✓ Pipeline completed successfully")
        print(f"Archive ID: {archive_id}")
        print(f"Extracted files: {len(member_ids)}")
        print(f"{'='*60}\n")
        
        return archive_id, member_ids
        
    finally:
        # Cleanup
        shutil.rmtree(temp_dir, ignore_errors=True)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python3 archive_pipeline.py <archive.zip>")
        sys.exit(1)
    
    archive_path = sys.argv[1]
    
    if not os.path.exists(archive_path):
        print(f"Error: File not found: {archive_path}")
        sys.exit(1)
    
    try:
        process_archive_pipeline(archive_path)
    except Exception as e:
        print(f"\n✗ Pipeline failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
