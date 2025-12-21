#!/usr/bin/env python3
"""
Ollama Model Importer for ThemisDB
==================================

Imports locally available Ollama models into ThemisDB.

Usage:
    python import_ollama_models.py [--ollama-path PATH] [--themis-url URL] [--token TOKEN]

Default Ollama path: C:\Users\mkrueger\.ollama\models (Windows)
                     ~/.ollama/models (Linux/Mac)
"""

import os
import sys
import json
import argparse
import hashlib
from pathlib import Path
from typing import List, Dict, Optional
import requests


class OllamaModelImporter:
    """Import Ollama models into ThemisDB"""
    
    def __init__(self, ollama_path: str, themis_url: str, auth_token: Optional[str] = None):
        self.ollama_path = Path(ollama_path)
        self.themis_url = themis_url.rstrip('/')
        self.auth_token = auth_token
        self.headers = {
            'Content-Type': 'application/json',
            'Authorization': f'Bearer {auth_token}' if auth_token else ''
        }
    
    def discover_models(self) -> List[Dict]:
        """Discover available Ollama models"""
        models = []
        
        if not self.ollama_path.exists():
            print(f"⚠️  Ollama path not found: {self.ollama_path}")
            return models
        
        # Ollama stores models in: manifests/registry.ollama.ai/{namespace}/{model}/{tag}
        manifests_path = self.ollama_path / "manifests" / "registry.ollama.ai"
        
        if not manifests_path.exists():
            print(f"⚠️  No Ollama manifests found in: {manifests_path}")
            return models
        
        # Scan for model manifests
        for namespace_dir in manifests_path.iterdir():
            if not namespace_dir.is_dir():
                continue
            
            for model_dir in namespace_dir.iterdir():
                if not model_dir.is_dir():
                    continue
                
                for tag_file in model_dir.iterdir():
                    if tag_file.is_file():
                        try:
                            with open(tag_file, 'r') as f:
                                manifest = json.load(f)
                            
                            model_name = f"{namespace_dir.name}/{model_dir.name}:{tag_file.name}"
                            model_info = {
                                'name': model_name,
                                'namespace': namespace_dir.name,
                                'model': model_dir.name,
                                'tag': tag_file.name,
                                'manifest': manifest,
                                'manifest_path': str(tag_file)
                            }
                            models.append(model_info)
                            print(f"✓ Found model: {model_name}")
                        except Exception as e:
                            print(f"⚠️  Error reading manifest {tag_file}: {e}")
        
        return models
    
    def get_blob_path(self, digest: str) -> Path:
        """Get path to blob file from digest"""
        # Ollama stores blobs in: blobs/{digest}
        return self.ollama_path / "blobs" / digest.replace('sha256:', 'sha256-')
    
    def upload_model_to_themis(self, model_info: Dict) -> bool:
        """Upload a model to ThemisDB"""
        model_name = model_info['name']
        manifest = model_info['manifest']
        
        print(f"\n📤 Uploading model: {model_name}")
        
        try:
            # Extract model metadata
            config_digest = manifest.get('config', {}).get('digest')
            if not config_digest:
                print(f"⚠️  No config digest found for {model_name}")
                return False
            
            # Read model config
            config_path = self.get_blob_path(config_digest)
            if not config_path.exists():
                print(f"⚠️  Config blob not found: {config_path}")
                return False
            
            with open(config_path, 'rb') as f:
                config_data = json.load(f)
            
            # Extract layers (GGUF weights)
            layers = manifest.get('layers', [])
            if not layers:
                print(f"⚠️  No layers found for {model_name}")
                return False
            
            # Find GGUF layer
            gguf_layer = None
            for layer in layers:
                if layer.get('mediaType') == 'application/vnd.ollama.image.model':
                    gguf_layer = layer
                    break
            
            if not gguf_layer:
                print(f"⚠️  No GGUF layer found for {model_name}")
                return False
            
            gguf_digest = gguf_layer['digest']
            gguf_path = self.get_blob_path(gguf_digest)
            
            if not gguf_path.exists():
                print(f"⚠️  GGUF blob not found: {gguf_path}")
                return False
            
            # Get file size
            file_size = gguf_path.stat().st_size
            print(f"  Model size: {file_size / (1024**3):.2f} GB")
            
            # Upload to ThemisDB via model ingestion API
            upload_url = f"{self.themis_url}/api/v1/llm/models/upload"
            
            # Stream upload in chunks
            chunk_size = 10 * 1024 * 1024  # 10 MB chunks
            
            # Create upload session
            session_data = {
                'model_id': f"{model_info['model']}:{model_info['tag']}",
                'model_name': model_info['model'],
                'format': 'gguf',
                'size': file_size,
                'metadata': {
                    'source': 'ollama',
                    'namespace': model_info['namespace'],
                    'tag': model_info['tag'],
                    'digest': gguf_digest,
                    'config': config_data
                }
            }
            
            response = requests.post(
                f"{upload_url}/session",
                json=session_data,
                headers=self.headers,
                timeout=30
            )
            
            if response.status_code != 200:
                print(f"❌ Failed to create upload session: {response.text}")
                return False
            
            session_id = response.json()['session_id']
            print(f"  Upload session: {session_id}")
            
            # Upload chunks
            with open(gguf_path, 'rb') as f:
                chunk_num = 0
                while True:
                    chunk = f.read(chunk_size)
                    if not chunk:
                        break
                    
                    chunk_num += 1
                    progress = (chunk_num * chunk_size) / file_size * 100
                    progress = min(progress, 100)
                    
                    print(f"  Uploading chunk {chunk_num} ({progress:.1f}%)", end='\r')
                    
                    response = requests.post(
                        f"{upload_url}/chunk",
                        data=chunk,
                        headers={
                            **self.headers,
                            'Content-Type': 'application/octet-stream',
                            'X-Session-ID': session_id,
                            'X-Chunk-Number': str(chunk_num)
                        },
                        timeout=300
                    )
                    
                    if response.status_code != 200:
                        print(f"\n❌ Failed to upload chunk {chunk_num}: {response.text}")
                        return False
            
            print(f"\n  Upload complete!")
            
            # Finalize upload
            response = requests.post(
                f"{upload_url}/finalize",
                json={'session_id': session_id},
                headers=self.headers,
                timeout=60
            )
            
            if response.status_code != 200:
                print(f"❌ Failed to finalize upload: {response.text}")
                return False
            
            result = response.json()
            model_urn = result.get('model_urn')
            
            print(f"✅ Model imported successfully!")
            print(f"   URN: {model_urn}")
            
            return True
            
        except Exception as e:
            print(f"❌ Error uploading model: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def import_models(self, model_filter: Optional[str] = None) -> Dict:
        """Import models from Ollama to ThemisDB"""
        models = self.discover_models()
        
        if not models:
            print("\n⚠️  No Ollama models found!")
            return {'total': 0, 'succeeded': 0, 'failed': 0}
        
        print(f"\n📦 Found {len(models)} Ollama model(s)")
        
        # Filter models if requested
        if model_filter:
            models = [m for m in models if model_filter in m['name']]
            print(f"   Filtered to {len(models)} model(s) matching '{model_filter}'")
        
        # Import each model
        results = {'total': len(models), 'succeeded': 0, 'failed': 0}
        
        for model in models:
            success = self.upload_model_to_themis(model)
            if success:
                results['succeeded'] += 1
            else:
                results['failed'] += 1
        
        return results


def get_default_ollama_path() -> str:
    """Get default Ollama models path based on OS"""
    if sys.platform == 'win32':
        # Windows default
        return str(Path.home() / '.ollama' / 'models')
    else:
        # Linux/Mac default
        return str(Path.home() / '.ollama' / 'models')


def main():
    parser = argparse.ArgumentParser(
        description='Import Ollama models into ThemisDB',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Import all models from default Ollama path
  python import_ollama_models.py
  
  # Import models from custom path
  python import_ollama_models.py --ollama-path /custom/path/.ollama/models
  
  # Import specific model
  python import_ollama_models.py --filter mistral
  
  # Use custom ThemisDB URL and auth token
  python import_ollama_models.py --themis-url http://localhost:8080 --token YOUR_TOKEN
        """
    )
    
    parser.add_argument(
        '--ollama-path',
        default=r'C:\Users\mkrueger\.ollama\models',
        help='Path to Ollama models directory (default: %(default)s)'
    )
    
    parser.add_argument(
        '--themis-url',
        default='http://localhost:7000',
        help='ThemisDB API URL (default: %(default)s)'
    )
    
    parser.add_argument(
        '--token',
        help='ThemisDB authentication token (Bearer token)'
    )
    
    parser.add_argument(
        '--filter',
        help='Only import models matching this string'
    )
    
    args = parser.parse_args()
    
    print("=" * 60)
    print("Ollama Model Importer for ThemisDB")
    print("=" * 60)
    print(f"Ollama path: {args.ollama_path}")
    print(f"ThemisDB URL: {args.themis_url}")
    print("=" * 60)
    
    # Create importer
    importer = OllamaModelImporter(
        ollama_path=args.ollama_path,
        themis_url=args.themis_url,
        auth_token=args.token
    )
    
    # Import models
    results = importer.import_models(model_filter=args.filter)
    
    # Print summary
    print("\n" + "=" * 60)
    print("Import Summary")
    print("=" * 60)
    print(f"Total models: {results['total']}")
    print(f"✅ Succeeded: {results['succeeded']}")
    print(f"❌ Failed: {results['failed']}")
    print("=" * 60)
    
    # Exit with appropriate code
    sys.exit(0 if results['failed'] == 0 else 1)


if __name__ == '__main__':
    main()
