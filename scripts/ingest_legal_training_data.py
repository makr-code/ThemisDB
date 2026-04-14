"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingest_legal_training_data.py                      ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:59:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     450                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41ed  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ThemisDB Legal Training Data Ingestion
========================================

Automatische Ingestion von HuggingFace German legal training data
Analog zu generate_docs_database.py

This script downloads German legal datasets from HuggingFace Hub and
exports them as JSON for import into ThemisDB.

Supported Datasets (in fallback order):
1. joelNiklaus/MultiLegalPile (German subset) - RECOMMENDED
   URL: https://huggingface.co/datasets/joelNiklaus/MultiLegalPile
2. joelito/legal_mc_de (DEPRECATED - may not be available)
   URL: https://huggingface.co/datasets/joelito/legal_mc_de  
3. elenanereiss/german-ler
   URL: https://huggingface.co/datasets/elenanereiss/german-ler
4. Local custom dataset (JSON file)

Usage:
    python3 scripts/ingest_legal_training_data.py
    python3 scripts/ingest_legal_training_data.py --output data/legal_training_data.json
    python3 scripts/ingest_legal_training_data.py --max-samples 5000
    python3 scripts/ingest_legal_training_data.py --dataset joelNiklaus/MultiLegalPile
    python3 scripts/ingest_legal_training_data.py --local-file custom_data.json
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


# Available German legal datasets (in order of preference)
LEGAL_DATASETS = [
    {
        "name": "joelNiklaus/MultiLegalPile",
        "url": "https://huggingface.co/datasets/joelNiklaus/MultiLegalPile",
        "language_filter": "de",  # Filter for German texts
        "description": "Multilingual legal corpus with German subset - RECOMMENDED",
        "split": "train",
        "text_field": "text",
        "config": "de"  # German configuration
    },
    {
        "name": "joelito/legal_mc_de",
        "url": "https://huggingface.co/datasets/joelito/legal_mc_de",
        "language_filter": None,
        "description": "German legal multiple choice (DEPRECATED - may not be available)",
        "split": "train",
        "text_field": "text",
        "config": None
    },
    {
        "name": "elenanereiss/german-ler",
        "url": "https://huggingface.co/datasets/elenanereiss/german-ler",
        "language_filter": None,
        "description": "German legal entity recognition dataset",
        "split": "train",
        "text_field": "text",
        "config": None
    }
]


def load_from_local_file(file_path: str) -> list:
    """
    Load legal training data from a local JSON file.
    
    Args:
        file_path: Path to local JSON file
        
    Returns:
        List of documents
    """
    logger.info(f"Loading data from local file: {file_path}")
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        # Support different JSON formats
        if isinstance(data, list):
            documents = data
        elif isinstance(data, dict) and 'documents' in data:
            documents = data['documents']
        elif isinstance(data, dict) and 'data' in data:
            documents = data['data']
        else:
            documents = [data]
        
        logger.info(f"✓ Loaded {len(documents)} documents from local file")
        return documents
        
    except Exception as e:
        logger.error(f"Error loading local file: {e}")
        return []


def ingest_legal_dataset(output_path: str, max_samples: int = 10000, dataset_name: str = None, local_file: str = None) -> bool:
    """
    Lädt legal Dataset von HuggingFace oder lokaler Datei und exportiert als JSON
    
    Args:
        output_path: Pfad zur Output JSON-Datei
        max_samples: Maximale Anzahl Samples (Standard: 10.000)
        dataset_name: Spezifischer Dataset-Name (optional, sonst automatische Auswahl)
        local_file: Pfad zu lokaler Daten-Datei (optional)
        
    Returns:
        True if successful, False otherwise
    """
    
    # Handle local file input
    if local_file:
        documents = load_from_local_file(local_file)
        if not documents:
            return False
        
        # Convert to ThemisDB format if needed
        formatted_docs = []
        for idx, doc in enumerate(documents[:max_samples]):
            if isinstance(doc, dict) and '_key' in doc:
                # Already in ThemisDB format
                formatted_docs.append(doc)
            else:
                # Convert to ThemisDB format
                text_content = doc.get('text', '') if isinstance(doc, dict) else str(doc)
                formatted_doc = {
                    "_key": f"legal_de_{idx:06d}",
                    "source": f"local_file:{local_file}",
                    "text": text_content,
                    "metadata": {
                        "index": idx,
                        "dataset": "local_custom",
                        "language": "de",
                        "domain": "legal"
                    }
                }
                formatted_docs.append(formatted_doc)
        
        documents = formatted_docs
        source_name = f"local_file:{local_file}"
    
    else:
        # Handle HuggingFace datasets
        try:
            from datasets import load_dataset, get_dataset_split_names
        except ImportError:
            logger.error("Error: datasets library not found!")
            logger.error("Please install: pip install datasets huggingface-hub")
            return False
        
        # Determine which dataset to use
        datasets_to_try = []
        if dataset_name:
            # User specified a dataset
            matching_dataset = next((d for d in LEGAL_DATASETS if d['name'] == dataset_name), None)
            if matching_dataset:
                datasets_to_try = [matching_dataset]
            else:
                # Custom dataset name provided
                datasets_to_try = [{
                    "name": dataset_name,
                    "url": f"https://huggingface.co/datasets/{dataset_name}",
                    "language_filter": None,
                    "description": "Custom dataset",
                    "split": "train",
                    "text_field": "text",
                    "config": None
                }]
        else:
            # Try datasets in fallback order
            datasets_to_try = LEGAL_DATASETS
        
        logger.info("")
        logger.info("=" * 60)
        logger.info("HuggingFace Legal Training Data Ingestion")
        logger.info("=" * 60)
        logger.info(f"Max samples: {max_samples}")
        logger.info("")
        
        dataset = None
        source_name = None
        dataset_info = None
        
        for dataset_info in datasets_to_try:
            try:
                logger.info(f"Trying dataset: {dataset_info['name']}")
                logger.info(f"URL: {dataset_info['url']}")
                logger.info(f"Description: {dataset_info['description']}")
                
                # Check available splits
                try:
                    available_splits = get_dataset_split_names(dataset_info['name'], config=dataset_info.get('config'))
                    logger.info(f"Available splits: {available_splits}")
                    
                    if dataset_info['split'] not in available_splits:
                        logger.warning(f"Split '{dataset_info['split']}' not found, trying first available split")
                        dataset_info['split'] = available_splits[0] if available_splits else 'train'
                except Exception as e:
                    logger.warning(f"Could not check splits: {e}")
                
                # Load dataset
                logger.info("Loading dataset from HuggingFace Hub...")
                load_kwargs = {"split": f"{dataset_info['split']}[:{max_samples}]"}
                if dataset_info.get('config'):
                    load_kwargs["name"] = dataset_info['config']
                
                dataset = load_dataset(dataset_info['name'], **load_kwargs)
                
                # Filter by language if needed
                if dataset_info.get('language_filter') and 'language' in dataset.column_names:
                    logger.info(f"Filtering for language: {dataset_info['language_filter']}")
                    dataset = dataset.filter(lambda x: x.get('language') == dataset_info['language_filter'])
                
                logger.info(f"✓ Successfully loaded {len(dataset)} samples from {dataset_info['name']}")
                source_name = f"huggingface:{dataset_info['name']}"
                break
                
            except Exception as e:
                logger.warning(f"Failed to load {dataset_info['name']}: {e}")
                logger.warning("Trying next dataset in fallback chain...")
                continue
        
        if dataset is None:
            logger.error("")
            logger.error("=" * 60)
            logger.error("ERROR: All dataset sources failed!")
            logger.error("=" * 60)
            logger.error("Tried datasets:")
            for d in datasets_to_try:
                logger.error(f"  - {d['name']} ({d['url']})")
            logger.error("")
            logger.error("Alternatives:")
            logger.error("  1. Use --dataset to specify a different HuggingFace dataset")
            logger.error("  2. Use --local-file to provide your own data")
            logger.error("  3. Check internet connection and HuggingFace access")
            logger.error("  4. Visit https://huggingface.co/datasets to search for datasets")
            logger.error("=" * 60)
            return False
        
        # Convert to ThemisDB format
        logger.info("")
        logger.info("Converting to ThemisDB format...")
        documents = []
        
        text_field = dataset_info.get('text_field', 'text')
        
        for idx, item in enumerate(dataset):
            # Extract text content - try different possible field names
            text_content = ""
            if text_field in item:
                text_content = item[text_field]
            elif 'text' in item:
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
                "source": source_name,
                "text": str(text_content),
                "metadata": {
                    "index": idx,
                    "dataset": dataset_info['name'],
                    "language": "de",
                    "domain": "legal"
                }
            }
            
            # Add any additional fields from the original dataset
            for key, value in item.items():
                if key not in ['text', 'content', 'document', text_field]:
                    try:
                        # Only add serializable values
                        json.dumps(value)
                        doc['metadata'][key] = value
                    except Exception as e:
                        logging.debug("Non-serializable metadata value skipped: %s", e)
            
            documents.append(doc)
            
            if (idx + 1) % 1000 == 0:
                logger.info(f"  Processed {idx + 1}/{len(dataset)} documents...")
        
        logger.info(f"✓ Converted {len(documents)} documents")
    
    # Prepare output data
    output_data = {
        "metadata": {
            "source": source_name,
            "version": "2.0.0",
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
    logger.info(f"Source: {source_name}")
    logger.info(f"Language: German (de)")
    logger.info(f"Domain: Legal")
    logger.info("=" * 60)
    
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Ingest German legal training data from HuggingFace or local files\n'
                    'Supports multiple datasets with automatic fallback\n\n'
                    'Primary: joelNiklaus/MultiLegalPile (German subset)\n'
                    'URL: https://huggingface.co/datasets/joelNiklaus/MultiLegalPile',
        formatter_class=argparse.RawDescriptionHelpFormatter
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
    parser.add_argument(
        '--dataset',
        type=str,
        default=None,
        help='Specific HuggingFace dataset to use (e.g., joelNiklaus/MultiLegalPile). '
             'If not specified, will try datasets in fallback order.'
    )
    parser.add_argument(
        '--local-file',
        type=str,
        default=None,
        help='Path to local JSON file with training data (alternative to HuggingFace datasets)'
    )
    parser.add_argument(
        '--list-datasets',
        action='store_true',
        help='List available datasets and exit'
    )
    
    args = parser.parse_args()
    
    if args.list_datasets:
        print("")
        print("=" * 60)
        print("Available German Legal Training Datasets")
        print("=" * 60)
        for i, dataset in enumerate(LEGAL_DATASETS, 1):
            print(f"\n{i}. {dataset['name']}")
            print(f"   URL: {dataset['url']}")
            print(f"   Description: {dataset['description']}")
            if dataset.get('config'):
                print(f"   Config: {dataset['config']}")
        print("")
        print("=" * 60)
        print("\nUsage:")
        print(f"  python3 scripts/ingest_legal_training_data.py --dataset {LEGAL_DATASETS[0]['name']}")
        print(f"  python3 scripts/ingest_legal_training_data.py --local-file custom_data.json")
        print("=" * 60)
        return 0
    
    success = ingest_legal_dataset(args.output, args.max_samples, args.dataset, args.local_file)
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
