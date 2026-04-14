"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_docs_database.py                          ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:31:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     251                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ThemisDB Documentation Database Generator
==========================================

This script generates a pre-compiled documentation database from ./docs and ./compendium
directories for use with the LLM-based documentation assistant.

The generated database can be used by administrators to query documentation
using the integrated llama.cpp LLM, enabling configuration assistance and
troubleshooting support.

Usage:
    python3 scripts/generate_docs_database.py
    python3 scripts/generate_docs_database.py --output data/docs_database.json
    python3 scripts/generate_docs_database.py --include-compendium
"""

import argparse
import json
import logging
import os
import sys
import io
from pathlib import Path
from datetime import datetime

# Fix encoding on Windows
if sys.platform == 'win32':
    import codecs
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, errors='replace')
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr.buffer, errors='replace')

# Add tools directory to path to import ingest module
SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent
TOOLS_DIR = REPO_ROOT / "tools"
sys.path.insert(0, str(TOOLS_DIR))

try:
    from ingest import IngestionConfig, IngestionEngine
except ImportError:
    print("Error: Could not import ingest module from tools/ingest.py")
    print("Please ensure tools/ingest.py is available")
    sys.exit(1)

# Configure logging with UTF-8 encoding
class UTF8StreamHandler(logging.StreamHandler):
    def __init__(self):
        super().__init__()
        # Force UTF-8 encoding with error handling
        # Force UTF-8 encoding on stderr
        if hasattr(sys.stderr, 'buffer'):
            self.stream = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')
        else:
            # stderr is already wrapped or is not a binary stream
            self.stream = sys.stderr

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('DocsDBGenerator')

# Replace handlers with UTF-8 safe handler
for handler in logger.handlers:
    logger.removeHandler(handler)
logger.addHandler(UTF8StreamHandler())


def generate_documentation_database(
    output_path: str = "data/docs_database.json",
    include_compendium: bool = False,
    include_examples: bool = False
) -> bool:
    """
    Generate a comprehensive documentation database from source files.
    
    Args:
        output_path: Path where the JSON database will be saved
        include_compendium: Whether to include compendium directory
        include_examples: Whether to include examples directory
        
    Returns:
        True if successful, False otherwise
    """
    
    docs_dir = REPO_ROOT / "docs"
    compendium_dir = REPO_ROOT / "compendium"
    examples_dir = REPO_ROOT / "examples"
    
    if not docs_dir.exists():
        logger.error(f"Documentation directory not found: {docs_dir}")
        return False
    
    sources = [str(docs_dir)]
    logger.info(f"[OK] Including docs directory: {docs_dir}")
    
    if include_compendium:
        if compendium_dir.exists():
            sources.append(str(compendium_dir))
            logger.info(f"[OK] Including compendium directory: {compendium_dir}")
        else:
            logger.warning(f"Compendium directory not found: {compendium_dir}")
    
    if include_examples:
        if examples_dir.exists():
            sources.append(str(examples_dir))
            logger.info(f"[OK] Including examples directory: {examples_dir}")
        else:
            logger.warning(f"Examples directory not found: {examples_dir}")
    
    supported_extensions = [
        '.md', '.txt', '.rst', '.json', '.yaml', '.yml',
        '.cpp', '.h', '.py', '.js', '.ts', '.sql'
    ]
    
    logger.info("=" * 60)
    logger.info("Processing source directories")
    logger.info("=" * 60)
    
    # Ingest from each source directory
    all_ingested_files = []
    total_stats = {
        'files_processed': 0,
        'files_skipped': 0,
        'files_failed': 0,
        'total_size_bytes': 0
    }

    for i, source_dir in enumerate(sources):
        try:
            logger.info(f"Processing directory {i + 1}/{len(sources)}: {source_dir}")
            config = IngestionConfig(
                source_dir=source_dir,
                output_file=f"{output_path}.tmp.{i}.json",
                include_extensions=supported_extensions
            )
            engine = IngestionEngine(config)
            result = engine.ingest()

            all_ingested_files.extend(result.get('ingested_files', []))

            stats = result.get('statistics', {})
            for key in total_stats:
                total_stats[key] += stats.get(key, 0)

            logger.info(f"Successfully ingested from {source_dir}")
        except Exception:
            import traceback
            logger.error(f"Failed to ingest from {source_dir}")
            logger.error(traceback.format_exc())
    # Prepare output data
    output_data = {
        'version': '1.0',
        'generated': datetime.now().isoformat(),
        'total_documents': len(all_ingested_files),
        'statistics': total_stats,
        'documents': all_ingested_files
    }
    
    # Write output file
    output_path_obj = Path(output_path)
    with open(output_path_obj, 'w', encoding='utf-8') as f:
        json.dump(output_data, f, indent=2, ensure_ascii=False)
    
    # Generate summary
    logger.info("")
    logger.info("=" * 60)
    logger.info("Documentation Database Generated Successfully!")
    logger.info("=" * 60)
    logger.info(f"Output file: {output_path_obj.absolute()}")
    logger.info(f"File size: {output_path_obj.stat().st_size / (1024 * 1024):.2f} MB")
    logger.info(f"Total documents: {len(all_ingested_files)}")
    logger.info(f"Files processed: {total_stats['files_processed']}")
    logger.info(f"Files skipped: {total_stats['files_skipped']}")
    logger.info(f"Files failed: {total_stats['files_failed']}")
    logger.info(f"Total content size: {total_stats['total_size_bytes'] / (1024 * 1024):.2f} MB")
    logger.info("=" * 60)
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Generate ThemisDB documentation database'
    )
    parser.add_argument(
        '--output',
        default='data/docs_database.json',
        help='Output path for documentation database JSON'
    )
    parser.add_argument(
        '--include-compendium',
        action='store_true',
        default=True,
        help='Include compendium directory'
    )
    parser.add_argument(
        '--include-examples',
        action='store_true',
        default=False,
        help='Include examples directory'
    )
    
    args = parser.parse_args()
    
    logger.info("")
    logger.info("=" * 60)
    logger.info("ThemisDB Documentation Database Generator")
    logger.info("=" * 60)
    
    success = generate_documentation_database(
        args.output,
        args.include_compendium,
        args.include_examples
    )
    
    if success:
        logger.info("Generation completed successfully!")
        return 0
    else:
        logger.error("Generation failed!")
        return 1


if __name__ == '__main__':
    sys.exit(main())
