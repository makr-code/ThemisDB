"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_docs_database_backup.py                   ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:07:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     298                                            ║
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
from pathlib import Path
from datetime import datetime

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

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('DocsDBGenerator')


def generate_documentation_database(
    output_path: str = "data/docs_database.json",
    include_compendium: bool = True,
    include_examples: bool = False
) -> bool:
    """
    Generate a documentation database from docs and optionally compendium directories.
    
    Args:
        output_path: Path where the documentation database will be saved
        include_compendium: Whether to include the compendium directory
        include_examples: Whether to include example documentation
        
    Returns:
        True if successful, False otherwise
    """
    
    logger.info("=" * 60)
    logger.info("ThemisDB Documentation Database Generator")
    logger.info("=" * 60)
    
    # Ensure output directory exists
    output_dir = Path(output_path).parent
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Define source directories
    docs_dir = REPO_ROOT / "docs"
    compendium_dir = REPO_ROOT / "compendium"
    examples_dir = REPO_ROOT / "examples"
    
    # Validate source directories
    if not docs_dir.exists():
        logger.error(f"Documentation directory not found: {docs_dir}")
        return False
    
    sources = [str(docs_dir)]
    logger.info(f"✓ Including docs directory: {docs_dir}")
    
    if include_compendium:
        if compendium_dir.exists():
            sources.append(str(compendium_dir))
            logger.info(f"✓ Including compendium directory: {compendium_dir}")
        else:
            logger.warning(f"Compendium directory not found: {compendium_dir}")
    
    if include_examples:
        if examples_dir.exists():
            sources.append(str(examples_dir))
            logger.info(f"✓ Including examples directory: {examples_dir}")
        else:
            logger.warning(f"Examples directory not found: {examples_dir}")
    
    # Configure ingestion for documentation
    # We'll process each directory separately and merge results
    all_ingested_files = []
    total_stats = {
        'total_files_scanned': 0,
        'files_processed': 0,
        'files_skipped': 0,
        'files_failed': 0,
        'total_size_bytes': 0
    }
    
    for source_dir in sources:
        logger.info(f"\n{'=' * 60}")
        logger.info(f"Processing: {source_dir}")
        logger.info('=' * 60)
        
        # Create a temporary database for tracking per directory
        db_path = f"/tmp/docs_ingestion_{Path(source_dir).name}.db"
        
        config = IngestionConfig(
            source_dir=source_dir,
            output_file=f"/tmp/docs_temp_{Path(source_dir).name}.json",
            db_path=db_path,
            include_extensions=['.md', '.txt', '.json', '.yaml', '.yml'],
            exclude_extensions=['.exe', '.dll', '.so', '.dylib', '.bin', '.png', '.jpg', '.jpeg', '.gif', '.svg', '.ico'],
            exclude_patterns=[
                '.git', '__pycache__', 'node_modules', '.venv', 
                'build', 'dist', 'output', 'temp', '.cache',
                'chrome', 'temp'  # Exclude compendium/chrome and compendium/temp
            ],
            max_file_size_mb=10.0,  # Reasonable size for documentation files
            extract_text_preview=True,
            preview_length=2000,  # Longer preview for documentation
            generate_vector_metadata=True,
            generate_graph_metadata=True,
            generate_relational_metadata=True
        )
        
        try:
            engine = IngestionEngine(config)
            result = engine.ingest()
            
            # Merge results
            if 'ingested_files' in result:
                all_ingested_files.extend(result['ingested_files'])
            
            # Aggregate statistics
            if 'statistics' in result:
                stats = result['statistics']
                total_stats['total_files_scanned'] += stats.get('total_files_scanned', 0)
                total_stats['files_processed'] += stats.get('files_processed', 0)
                total_stats['files_skipped'] += stats.get('files_skipped', 0)
                total_stats['files_failed'] += stats.get('files_failed', 0)
                total_stats['total_size_bytes'] += stats.get('total_size_bytes', 0)
            
            # Clean up temporary files
            engine.tracker.close()
            if os.path.exists(db_path):
                os.remove(db_path)
            if os.path.exists(config.output_file):
                os.remove(config.output_file)
                
        except Exception as e:
            logger.error(f"Error processing {source_dir}: {e}", exc_info=True)
            return False
    
    # Create final output with metadata
    logger.info(f"\n{'=' * 60}")
    logger.info("Generating final documentation database...")
    logger.info('=' * 60)
    
    output_data = {
        'metadata': {
            'version': '1.0.0',
            'database_type': 'documentation',
            'generation_time': datetime.now().isoformat(),
            'sources': sources,
            'total_documents': len(all_ingested_files),
            'themisdb_version': _get_themisdb_version(),
            'description': 'Pre-compiled ThemisDB documentation database for LLM-based assistance'
        },
        'statistics': total_stats,
        'documents': all_ingested_files
    }
    
    # Write output file
    output_path_obj = Path(output_path)
    with open(output_path_obj, 'w', encoding='utf-8') as f:
        json.dump(output_data, f, indent=2, ensure_ascii=False)
    
    # Generate summary
    logger.info("\n" + "=" * 60)
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


def _get_themisdb_version() -> str:
    """Get ThemisDB version from VERSION file"""
    version_file = REPO_ROOT / "VERSION"
    if version_file.exists():
        return version_file.read_text().strip()
    return "unknown"


def main():
    parser = argparse.ArgumentParser(
        description='Generate pre-compiled documentation database for ThemisDB LLM assistant',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate documentation database with default settings
  python3 scripts/generate_docs_database.py
  
  # Specify custom output path
  python3 scripts/generate_docs_database.py --output /path/to/docs_db.json
  
  # Generate with docs only (exclude compendium)
  python3 scripts/generate_docs_database.py --no-compendium
  
  # Include example documentation
  python3 scripts/generate_docs_database.py --include-examples
        """
    )
    
    parser.add_argument(
        '--output',
        type=str,
        default='data/docs_database.json',
        help='Output path for documentation database (default: data/docs_database.json)'
    )
    
    parser.add_argument(
        '--no-compendium',
        action='store_true',
        help='Exclude compendium directory from database'
    )
    
    parser.add_argument(
        '--include-examples',
        action='store_true',
        help='Include examples directory in database'
    )
    
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Enable verbose logging'
    )
    
    args = parser.parse_args()
    
    # Set logging level
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Generate database
    success = generate_documentation_database(
        output_path=args.output,
        include_compendium=not args.no_compendium,
        include_examples=args.include_examples
    )
    
    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
