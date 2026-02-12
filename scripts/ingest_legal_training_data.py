#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ThemisDB Legal Training Data Ingestion
========================================

Automatische Ingestion von HuggingFace legal-bert-de Trainingsdaten
Analog zu generate_docs_database.py

This script downloads the legal-bert-de dataset from HuggingFace Hub and
exports it as JSON for import into ThemisDB.

Usage:
    python3 scripts/ingest_legal_training_data.py
    python3 scripts/ingest_legal_training_data.py --output data/legal_training_data.json
    python3 scripts/ingest_legal_training_data.py --max-samples 5000
"""

import os
import json
import argparse
import logging
import sys
import io
from pathlib import Path
from datetime import datetime

# Fix encoding on Windows
if sys.platform == 'win32':
    import codecs
    if hasattr(sys.stdout, 'buffer'):
        sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, errors='replace')
    if hasattr(sys.stderr, 'buffer'):
        sys.stderr = codecs.getwriter('utf-8')(sys.stderr.buffer, errors='replace')

# Configure logging with UTF-8 encoding
class UTF8StreamHandler(logging.StreamHandler):
    def __init__(self):
        super().__init__()
        # Force UTF-8 encoding with error handling
        if sys.platform == 'win32' and hasattr(sys.stderr, 'buffer'):
            self.stream = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger('LegalDataIngestion')
for handler in logger.handlers:
    logger.removeHandler(handler)
logger.addHandler(UTF8StreamHandler())


def ingest_legal_dataset(output_path: str, max_samples: int = 10000) -> bool:
    """
    Lädt legal-bert-de Dataset von HuggingFace und exportiert als JSON
    
    Args:
        output_path: Pfad zur Output JSON-Datei
        max_samples: Maximale Anzahl Samples (Standard: 10.000)
        
    Returns:
        True if successful, False otherwise
    """
    
    try:
        from datasets import load_dataset
    except ImportError:
        logger.error("Error: datasets library not found!")
        logger.error("Please install: pip install datasets huggingface-hub")
        return False
    
    logger.info("")
    logger.info("=" * 60)
    logger.info("HuggingFace Legal Training Data Ingestion")
    logger.info("=" * 60)
    logger.info(f"Dataset: joelito/legal_mc_de")
    logger.info(f"Max samples: {max_samples}")
    logger.info("")
    
    try:
        # Load dataset from HuggingFace
        logger.info("Loading dataset from HuggingFace Hub...")
        logger.info("(This may take a few minutes on first run)")
        
        dataset = load_dataset("joelito/legal_mc_de", split=f"train[:{max_samples}]")
        logger.info(f"✓ Loaded {len(dataset)} samples from HuggingFace")
        
    except Exception as e:
        logger.error(f"Error loading dataset: {e}")
        logger.error("Make sure you have internet connection and the dataset exists")
        return False
    
    # Convert to ThemisDB format
    logger.info("")
    logger.info("Converting to ThemisDB format...")
    documents = []
    
    for idx, item in enumerate(dataset):
        # Extract text content - try different possible field names
        text_content = ""
        if 'text' in item:
            text_content = item['text']
        elif 'content' in item:
            text_content = item['content']
        elif 'document' in item:
            text_content = item['document']
        else:
            # If no standard field, convert entire item to JSON
            text_content = json.dumps(item, ensure_ascii=False)
        
        # Create document in ThemisDB format
        doc = {
            "_key": f"legal_de_{idx:06d}",
            "source": "huggingface:joelito/legal_mc_de",
            "text": str(text_content),
            "metadata": {
                "index": idx,
                "dataset": "legal_mc_de",
                "language": "de",
                "domain": "legal"
            }
        }
        
        # Add any additional fields from the original dataset
        for key, value in item.items():
            if key not in ['text', 'content', 'document']:
                doc['metadata'][key] = value
        
        documents.append(doc)
        
        if (idx + 1) % 1000 == 0:
            logger.info(f"  Processed {idx + 1}/{len(dataset)} documents...")
    
    logger.info(f"✓ Converted {len(documents)} documents")
    
    # Prepare output data
    output_data = {
        "metadata": {
            "source": "huggingface:joelito/legal_mc_de",
            "version": "1.0.0",
            "generated": datetime.now().isoformat(),
            "count": len(documents),
            "max_samples": max_samples
        },
        "documents": documents
    }
    
    # Ensure output directory exists
    output_path_obj = Path(output_path)
    output_path_obj.parent.mkdir(parents=True, exist_ok=True)
    
    # Write JSON file
    logger.info("")
    logger.info("Writing JSON output...")
    with open(output_path_obj, 'w', encoding='utf-8') as f:
        json.dump(output_data, f, ensure_ascii=False, indent=2)
    
    # Generate summary
    file_size_mb = output_path_obj.stat().st_size / (1024 * 1024)
    
    logger.info("")
    logger.info("=" * 60)
    logger.info("Legal Training Data Ingestion Complete!")
    logger.info("=" * 60)
    logger.info(f"Output file: {output_path_obj.absolute()}")
    logger.info(f"File size: {file_size_mb:.2f} MB")
    logger.info(f"Total documents: {len(documents)}")
    logger.info(f"Dataset: joelito/legal_mc_de")
    logger.info(f"Language: German (de)")
    logger.info(f"Domain: Legal")
    logger.info("=" * 60)
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Ingest HuggingFace legal-bert-de training data'
    )
    parser.add_argument(
        '--output',
        default='data/legal_training_data.json',
        help='Output path for JSON export (default: data/legal_training_data.json)'
    )
    parser.add_argument(
        '--max-samples',
        type=int,
        default=10000,
        help='Maximum number of samples to ingest (default: 10000)'
    )
    
    args = parser.parse_args()
    
    success = ingest_legal_dataset(args.output, args.max_samples)
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
