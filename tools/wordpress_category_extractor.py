"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wordpress_category_extractor.py                    ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:08:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     539                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
WordPress Category and Tag Extractor for ThemisDB Documentation
================================================================

Intelligently extracts meaningful categories and tags from markdown files
for WordPress import, avoiding dates, file paths, and other non-semantic information.

Features:
- Semantic category extraction from file paths and content
- YAML frontmatter support for explicit metadata
- Filtering of dates, numbers, and non-useful categories
- WordPress API integration for checking existing categories
- Configurable category mapping rules

Usage:
    python3 tools/wordpress_category_extractor.py --docs-path docs/ --output wp_categories.json
    python3 tools/wordpress_category_extractor.py --check-wordpress --wp-url https://yoursite.com
"""

import argparse
import json
import logging
import re
from pathlib import Path
from typing import Dict, List, Set, Optional, Any
from dataclasses import dataclass, asdict, field
from datetime import datetime
import hashlib

try:
    import yaml
except ImportError:
    yaml = None

try:
    import requests
except ImportError:
    requests = None


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('WP-Category-Extractor')


# Semantic category mapping from directory structure to meaningful categories
CATEGORY_MAPPING = {
    'architecture': 'Architecture',
    'aql': 'AQL Query Language',
    'apis': 'API Reference',
    'deployment': 'Deployment',
    'development': 'Development',
    'enterprise': 'Enterprise Features',
    'features': 'Features',
    'geo': 'Geospatial',
    'guides': 'Guides',
    'integrations': 'Integrations',
    'llm': 'LLM Integration',
    'observability': 'Monitoring & Observability',
    'performance': 'Performance',
    'plugins': 'Plugins',
    'query': 'Query Language',
    'releases': 'Release Notes',
    'security': 'Security',
    'server': 'Server',
    'storage': 'Storage',
    'timeseries': 'Time-Series',
    'analytics': 'Analytics',
    'auth': 'Authentication',
    'clients': 'Client SDKs',
    'compliance': 'Compliance',
    'connectors': 'Connectors',
    'governance': 'Governance',
    'legal': 'Legal',
    'operations': 'Operations',
    'roadmap': 'Roadmap',
    'scheduler': 'Scheduler',
    'search': 'Search',
    'sharding': 'Sharding',
    'tools': 'Tools',
    'strategie': 'Strategy',
    'admin_tools': 'Admin Tools',
    'content': 'Content Management',
    'policies': 'Policies',
    'projects': 'Projects',
    'reports': 'Reports',
    'research': 'Research',
    'audit': 'Audit',
    'docs': 'Documentation',
}

# Words to exclude from categories (dates, common words, etc.)
EXCLUDE_PATTERNS = [
    r'^\d+$',  # Pure numbers
    r'^\d{4}$',  # Years
    r'^v?\d+\.\d+',  # Version numbers
    r'(?i)^(januar|februar|märz|april|mai|juni|juli|august|september|oktober|november|dezember)$',  # Months in German
    r'(?i)^(january|february|march|april|may|june|july|august|september|october|november|december)$',  # Months in English
    r'^\d+\s+\d+$',  # Patterns like "9 2026"
    r'^use$',  # Generic word "use"
    r'^de$|^en$|^fr$|^es$|^ja$',  # Language codes
    r'^readme$|^index$',  # Common file names
    r'(?i)^https?$',  # Protocol names
]

# Key topics to extract as tags from content
KEY_TOPICS = [
    'vector search', 'graph database', 'time-series', 'llm', 'ai', 'machine learning',
    'raid', 'replication', 'backup', 'security', 'encryption', 'authentication',
    'docker', 'kubernetes', 'monitoring', 'metrics', 'performance', 'optimization',
    'multi-model', 'nosql', 'sql', 'query', 'api', 'rest', 'grpc',
    'json', 'binary', 'protocol', 'client', 'sdk', 'integration',
    'hypertable', 'analytics', 'olap', 'streaming', 'batch',
    'infrastructure', 'governance', 'compliance', 'gdpr',
]


@dataclass
class DocumentMetadata:
    """Metadata for a documentation file"""
    file_path: str
    relative_path: str
    title: str
    categories: List[str] = field(default_factory=list)
    tags: List[str] = field(default_factory=list)
    language: str = 'de'
    date_created: Optional[str] = None
    date_modified: Optional[str] = None
    content_hash: str = ''
    
    def to_dict(self) -> dict:
        return asdict(self)


class CategoryExtractor:
    """Extracts meaningful categories and tags from markdown documentation"""
    
    def __init__(self, docs_path: str, wp_url: Optional[str] = None, wp_user: Optional[str] = None, wp_password: Optional[str] = None):
        self.docs_path = Path(docs_path).resolve()
        
        # Security: Validate that docs_path exists and is a directory
        if not self.docs_path.exists():
            raise ValueError(f"Documentation path does not exist: {docs_path}")
        if not self.docs_path.is_dir():
            raise ValueError(f"Documentation path is not a directory: {docs_path}")
        
        # Security: Basic validation to prevent system path traversal
        # Ensure path doesn't start with suspicious directories
        path_str = str(self.docs_path)
        suspicious_prefixes = ['/etc', '/proc', '/sys', '/dev', '/root', '/var/log', '/usr/bin', '/usr/sbin']
        for prefix in suspicious_prefixes:
            if path_str.startswith(prefix):
                raise ValueError(f"Documentation path in restricted system directory: {docs_path}")
        
        self.wp_url = wp_url
        self.wp_user = wp_user
        self.wp_password = wp_password
        self.existing_categories: Set[str] = set()
        self.existing_tags: Set[str] = set()
        
    def fetch_existing_wordpress_categories(self) -> Set[str]:
        """Fetch existing categories from WordPress via REST API"""
        if not self.wp_url or not requests:
            logger.warning("WordPress URL not provided or requests library not available")
            return set()
        
        try:
            api_url = f"{self.wp_url.rstrip('/')}/wp-json/wp/v2/categories"
            params = {'per_page': 100}
            categories = set()
            
            response = requests.get(api_url, params=params, timeout=10)
            if response.status_code == 200:
                for cat in response.json():
                    categories.add(cat['name'])
                logger.info(f"Fetched {len(categories)} existing categories from WordPress")
                return categories
            else:
                logger.error(f"Failed to fetch categories: HTTP {response.status_code}")
        except Exception as e:
            logger.error(f"Error fetching WordPress categories: {e}")
        
        return set()
    
    def fetch_existing_wordpress_tags(self) -> Set[str]:
        """Fetch existing tags from WordPress via REST API"""
        if not self.wp_url or not requests:
            logger.warning("WordPress URL not provided or requests library not available")
            return set()
        
        try:
            api_url = f"{self.wp_url.rstrip('/')}/wp-json/wp/v2/tags"
            params = {'per_page': 100}
            tags = set()
            
            response = requests.get(api_url, params=params, timeout=10)
            if response.status_code == 200:
                for tag in response.json():
                    tags.add(tag['name'])
                logger.info(f"Fetched {len(tags)} existing tags from WordPress")
                return tags
            else:
                logger.error(f"Failed to fetch tags: HTTP {response.status_code}")
        except Exception as e:
            logger.error(f"Error fetching WordPress tags: {e}")
        
        return set()
    
    def is_valid_category(self, category: str) -> bool:
        """Check if a category name is valid (not a date, number, etc.)"""
        category = category.strip()
        
        # Empty or too short
        if not category or len(category) < 2:
            return False
        
        # Check against exclude patterns
        for pattern in EXCLUDE_PATTERNS:
            if re.match(pattern, category, re.IGNORECASE):
                return False
        
        # Reject if it's mostly numbers
        if sum(c.isdigit() for c in category) / len(category) > 0.5:
            return False
        
        return True
    
    def extract_categories_from_path(self, relative_path: Path) -> List[str]:
        """Extract meaningful categories from file path"""
        categories = []
        
        # Also check the last part of docs_path for categories
        # Only process the final directory name to avoid system paths
        if len(self.docs_path.parts) > 0:
            last_part = self.docs_path.parts[-1]
            if last_part.lower() in CATEGORY_MAPPING:
                categories.append(CATEGORY_MAPPING[last_part.lower()])
            elif self.is_valid_category(last_part):
                clean_part = last_part.replace('_', ' ').replace('-', ' ').title()
                categories.append(clean_part)
        
        # Process each directory in the relative path
        parts = relative_path.parts
        for part in parts[:-1]:  # Exclude the filename itself
            # Check if we have a semantic mapping
            if part.lower() in CATEGORY_MAPPING:
                categories.append(CATEGORY_MAPPING[part.lower()])
            elif self.is_valid_category(part):
                # Capitalize and clean up the name
                clean_part = part.replace('_', ' ').replace('-', ' ').title()
                categories.append(clean_part)
        
        return list(set(categories))  # Remove duplicates
    
    def extract_frontmatter(self, content: str) -> Dict[str, Any]:
        """Extract YAML frontmatter from markdown content"""
        if not yaml or not content.startswith('---'):
            return {}
        
        # Find the end of frontmatter
        lines = content.split('\n')
        if len(lines) < 3:
            return {}
        
        end_idx = -1
        for i, line in enumerate(lines[1:], 1):
            if line.strip() == '---':
                end_idx = i
                break
        
        if end_idx == -1:
            return {}
        
        try:
            frontmatter_text = '\n'.join(lines[1:end_idx])
            return yaml.safe_load(frontmatter_text) or {}
        except Exception as e:
            logger.warning(f"Failed to parse frontmatter: {e}")
            return {}
    
    def extract_title_from_content(self, content: str) -> str:
        """Extract title from markdown content"""
        # Try to find first h1 heading
        lines = content.split('\n')
        for line in lines:
            line = line.strip()
            if line.startswith('# '):
                return line[2:].strip()
        
        return "Untitled"
    
    def extract_tags_from_content(self, content: str) -> List[str]:
        """Extract relevant tags from content based on key topics"""
        tags = []
        content_lower = content.lower()
        
        for topic in KEY_TOPICS:
            if topic.lower() in content_lower:
                # Format the tag nicely
                tag = topic.title() if '-' not in topic else topic.upper()
                tags.append(tag)
        
        # Limit to most relevant tags
        return tags[:10]
    
    def extract_language_from_path(self, relative_path: Path) -> str:
        """Determine document language from path"""
        parts = relative_path.parts
        if len(parts) > 0:
            first_part = parts[0].lower()
            if first_part in ['de', 'en', 'fr', 'es', 'ja']:
                return first_part
        return 'de'  # Default to German
    
    def process_markdown_file(self, file_path: Path) -> DocumentMetadata:
        """Process a single markdown file and extract metadata"""
        relative_path = file_path.relative_to(self.docs_path)
        
        # Read file content
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
        except Exception as e:
            logger.error(f"Error reading {file_path}: {e}")
            return None
        
        # Extract frontmatter if present
        frontmatter = self.extract_frontmatter(content)
        
        # Extract metadata
        title = frontmatter.get('title') or self.extract_title_from_content(content)
        categories = frontmatter.get('categories', [])
        tags = frontmatter.get('tags', [])
        
        # If no categories in frontmatter, extract from path
        if not categories:
            categories = self.extract_categories_from_path(relative_path)
        
        # If no tags in frontmatter, extract from content
        if not tags:
            tags = self.extract_tags_from_content(content)
        
        # Filter categories
        categories = [cat for cat in categories if self.is_valid_category(cat)]
        
        # Extract language
        language = frontmatter.get('language') or self.extract_language_from_path(relative_path)
        
        # Get file modification time
        date_modified = datetime.fromtimestamp(file_path.stat().st_mtime).isoformat()
        
        # Calculate content hash
        content_hash = hashlib.sha256(content.encode('utf-8')).hexdigest()[:12]
        
        return DocumentMetadata(
            file_path=str(file_path),
            relative_path=str(relative_path),
            title=title,
            categories=categories,
            tags=tags,
            language=language,
            date_modified=date_modified,
            content_hash=content_hash
        )
    
    def process_all_documents(self) -> List[DocumentMetadata]:
        """Process all markdown files in the documentation directory"""
        documents = []
        
        logger.info(f"Scanning {self.docs_path} for markdown files...")
        md_files = list(self.docs_path.rglob('*.md'))
        logger.info(f"Found {len(md_files)} markdown files")
        
        for md_file in md_files:
            # Skip certain directories
            if any(part.startswith('.') for part in md_file.parts):
                continue
            if 'archive' in md_file.parts or 'compiled' in md_file.parts:
                continue
            
            doc_meta = self.process_markdown_file(md_file)
            if doc_meta:
                documents.append(doc_meta)
        
        logger.info(f"Processed {len(documents)} documents")
        return documents
    
    def get_category_summary(self, documents: List[DocumentMetadata]) -> Dict[str, int]:
        """Get summary of all categories and their usage count"""
        category_counts = {}
        for doc in documents:
            for category in doc.categories:
                category_counts[category] = category_counts.get(category, 0) + 1
        
        return dict(sorted(category_counts.items(), key=lambda x: x[1], reverse=True))
    
    def get_tag_summary(self, documents: List[DocumentMetadata]) -> Dict[str, int]:
        """Get summary of all tags and their usage count"""
        tag_counts = {}
        for doc in documents:
            for tag in doc.tags:
                tag_counts[tag] = tag_counts.get(tag, 0) + 1
        
        return dict(sorted(tag_counts.items(), key=lambda x: x[1], reverse=True))
    
    def generate_wordpress_import_data(self, documents: List[DocumentMetadata]) -> Dict:
        """Generate WordPress import data structure"""
        return {
            'metadata': {
                'generated_at': datetime.now().isoformat(),
                'total_documents': len(documents),
                'source_path': str(self.docs_path),
            },
            'categories': self.get_category_summary(documents),
            'tags': self.get_tag_summary(documents),
            'documents': [doc.to_dict() for doc in documents]
        }


def main():
    parser = argparse.ArgumentParser(
        description='Extract meaningful categories and tags from ThemisDB documentation for WordPress'
    )
    parser.add_argument(
        '--docs-path',
        type=str,
        default='docs',
        help='Path to documentation directory (default: docs)'
    )
    parser.add_argument(
        '--output',
        type=str,
        default='wordpress_categories.json',
        help='Output JSON file (default: wordpress_categories.json)'
    )
    parser.add_argument(
        '--check-wordpress',
        action='store_true',
        help='Check existing WordPress categories and tags'
    )
    parser.add_argument(
        '--wp-url',
        type=str,
        help='WordPress site URL (e.g., https://yoursite.com)'
    )
    parser.add_argument(
        '--wp-user',
        type=str,
        help='WordPress username for authentication'
    )
    parser.add_argument(
        '--wp-password',
        type=str,
        help='WordPress password for authentication'
    )
    
    args = parser.parse_args()
    
    # Create extractor
    extractor = CategoryExtractor(
        docs_path=args.docs_path,
        wp_url=args.wp_url,
        wp_user=args.wp_user,
        wp_password=args.wp_password
    )
    
    # Fetch existing WordPress categories/tags if requested
    if args.check_wordpress:
        if not args.wp_url:
            logger.error("--wp-url is required when using --check-wordpress")
            return 1
        
        logger.info("Fetching existing WordPress categories and tags...")
        extractor.existing_categories = extractor.fetch_existing_wordpress_categories()
        extractor.existing_tags = extractor.fetch_existing_wordpress_tags()
    
    # Process all documents
    documents = extractor.process_all_documents()
    
    # Generate WordPress import data
    import_data = extractor.generate_wordpress_import_data(documents)
    
    # Save to output file
    output_path = Path(args.output)
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(import_data, f, indent=2, ensure_ascii=False)
    
    logger.info(f"WordPress import data saved to {output_path}")
    
    # Print summary
    print("\n" + "="*60)
    print("CATEGORY EXTRACTION SUMMARY")
    print("="*60)
    print(f"\nTotal documents processed: {len(documents)}")
    print(f"\nTotal unique categories: {len(import_data['categories'])}")
    print("\nTop 10 categories:")
    for i, (category, count) in enumerate(list(import_data['categories'].items())[:10], 1):
        existing = " (EXISTS in WP)" if category in extractor.existing_categories else " (NEW)"
        print(f"  {i}. {category}: {count} docs{existing}")
    
    print(f"\nTotal unique tags: {len(import_data['tags'])}")
    print("\nTop 10 tags:")
    for i, (tag, count) in enumerate(list(import_data['tags'].items())[:10], 1):
        existing = " (EXISTS in WP)" if tag in extractor.existing_tags else " (NEW)"
        print(f"  {i}. {tag}: {count} docs{existing}")
    
    print("\n" + "="*60)
    print(f"Output saved to: {output_path}")
    print("="*60 + "\n")
    
    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main())
