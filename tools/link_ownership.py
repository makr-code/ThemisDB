"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            link_ownership.py                                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     418                                            ║
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
ThemisDB Ownership Linkage Tool
================================

Creates ownership and visibility relationships between PostgreSQL-imported entities
and Active Directory groups/users. This enables RBAC-based access control through
Apache Ranger policies that reference AD groups.

Features:
- Links PostgreSQL tables/schemas to AD groups via OWNED_BY edges
- Creates VISIBLE_TO edges for read access control
- Supports mapping files (CSV/YAML) for explicit ownership rules
- Supports naming conventions for automatic mapping
- Generates JSONL output compatible with tools/ingest.py

Usage:
    python3 tools/link_ownership.py --mapping ownership_mapping.yaml --output ownership_edges.jsonl
    python3 tools/link_ownership.py --convention "table_name_pattern" --output ownership_edges.jsonl
"""

import argparse
import csv
import hashlib
import json
import logging
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import List, Dict, Set, Optional, Any, Tuple

try:
    import yaml
except ImportError:
    yaml = None


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('ownership_linkage.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('ThemisDB-Ownership')


class OwnershipMapping:
    """Represents an ownership mapping rule"""
    def __init__(self, 
                 entity_pattern: str,
                 owner_group: str,
                 visible_to_groups: List[str] = None,
                 entity_type: str = "postgres_table",
                 attributes: Dict[str, Any] = None):
        self.entity_pattern = entity_pattern
        self.owner_group = owner_group
        self.visible_to_groups = visible_to_groups or []
        self.entity_type = entity_type
        self.attributes = attributes or {}


class OwnershipLinker:
    """Creates ownership and visibility edges between entities and AD groups"""
    
    def __init__(self, mappings: List[OwnershipMapping]):
        self.mappings = mappings
        self.created_edges = 0
        self.matched_entities = set()
    
    def _create_edge(self, edge_type: str, source_id: str, target_id: str, 
                     attributes: Dict[str, Any] = None) -> Dict[str, Any]:
        """Create an edge entity in ThemisDB format"""
        edge_id = f"edge:{hashlib.sha256(f'{source_id}-{edge_type}-{target_id}'.encode()).hexdigest()[:16]}"
        
        edge = {
            "id": edge_id,
            "type": edge_type,
            "source": source_id,
            "target": target_id,
            "attributes": attributes or {},
            "metadata": {
                "source": "ownership_linker",
                "created_time": datetime.now(timezone.utc).isoformat(),
                "edge_type": edge_type
            }
        }
        
        return edge
    
    def _match_entity(self, entity_id: str, pattern: str) -> bool:
        """Check if entity ID matches the pattern"""
        # Support wildcards and regex
        if '*' in pattern:
            # Convert glob pattern to regex
            regex_pattern = pattern.replace('.', r'\.').replace('*', '.*')
            return bool(re.match(f'^{regex_pattern}$', entity_id))
        else:
            # Exact match or prefix match
            return entity_id == pattern or entity_id.startswith(pattern + ':')
    
    def create_ownership_edges(self, entities: List[str], output_file) -> int:
        """Create OWNED_BY edges based on mappings"""
        count = 0
        
        for entity_id in entities:
            for mapping in self.mappings:
                if self._match_entity(entity_id, mapping.entity_pattern):
                    # Create OWNED_BY edge
                    owner_target = f"ad_group:{mapping.owner_group}" if not mapping.owner_group.startswith('ad_') else mapping.owner_group
                    
                    edge = self._create_edge(
                        'OWNED_BY',
                        entity_id,
                        owner_target,
                        mapping.attributes
                    )
                    
                    output_file.write(json.dumps(edge) + '\n')
                    count += 1
                    self.matched_entities.add(entity_id)
                    
                    logger.debug(f"Created OWNED_BY edge: {entity_id} -> {owner_target}")
        
        return count
    
    def create_visibility_edges(self, entities: List[str], output_file) -> int:
        """Create VISIBLE_TO edges based on mappings"""
        count = 0
        
        for entity_id in entities:
            for mapping in self.mappings:
                if self._match_entity(entity_id, mapping.entity_pattern):
                    # Create VISIBLE_TO edges for each group
                    for group in mapping.visible_to_groups:
                        target = f"ad_group:{group}" if not group.startswith('ad_') else group
                        
                        edge = self._create_edge(
                            'VISIBLE_TO',
                            entity_id,
                            target,
                            {'access_level': 'read'}
                        )
                        
                        output_file.write(json.dumps(edge) + '\n')
                        count += 1
                        
                        logger.debug(f"Created VISIBLE_TO edge: {entity_id} -> {target}")
        
        return count
    
    def link_all(self, entities: List[str], output_path: str) -> bool:
        """Create all ownership and visibility edges"""
        try:
            logger.info(f"Creating ownership edges for {len(entities)} entities")
            logger.info(f"Using {len(self.mappings)} mapping rules")
            
            with open(output_path, 'w', encoding='utf-8') as f:
                # Create ownership edges
                owned_count = self.create_ownership_edges(entities, f)
                
                # Create visibility edges
                visible_count = self.create_visibility_edges(entities, f)
                
                self.created_edges = owned_count + visible_count
            
            logger.info("=" * 60)
            logger.info("Ownership Linkage Summary:")
            logger.info(f"  OWNED_BY edges:   {owned_count}")
            logger.info(f"  VISIBLE_TO edges: {visible_count}")
            logger.info(f"  Total edges:      {self.created_edges}")
            logger.info(f"  Matched entities: {len(self.matched_entities)}")
            logger.info(f"  Unmatched:        {len(entities) - len(self.matched_entities)}")
            logger.info(f"Output file: {output_path}")
            logger.info("=" * 60)
            
            return True
            
        except Exception as e:
            logger.error(f"Linkage failed: {e}", exc_info=True)
            return False


def load_mappings_from_yaml(yaml_path: str) -> List[OwnershipMapping]:
    """Load ownership mappings from YAML file"""
    if not yaml:
        raise ImportError("PyYAML not installed. Install with: pip install pyyaml")
    
    try:
        with open(yaml_path, 'r') as f:
            config = yaml.safe_load(f)
        
        mappings = []
        for item in config.get('mappings', []):
            mapping = OwnershipMapping(
                entity_pattern=item['entity_pattern'],
                owner_group=item['owner_group'],
                visible_to_groups=item.get('visible_to_groups', []),
                entity_type=item.get('entity_type', 'postgres_table'),
                attributes=item.get('attributes', {})
            )
            mappings.append(mapping)
        
        logger.info(f"Loaded {len(mappings)} mapping rules from {yaml_path}")
        return mappings
        
    except Exception as e:
        logger.error(f"Failed to load mappings from {yaml_path}: {e}")
        raise


def load_mappings_from_csv(csv_path: str) -> List[OwnershipMapping]:
    """Load ownership mappings from CSV file"""
    try:
        mappings = []
        
        with open(csv_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                visible_to = row.get('visible_to_groups', '').split(';') if row.get('visible_to_groups') else []
                visible_to = [g.strip() for g in visible_to if g.strip()]
                
                mapping = OwnershipMapping(
                    entity_pattern=row['entity_pattern'],
                    owner_group=row['owner_group'],
                    visible_to_groups=visible_to,
                    entity_type=row.get('entity_type', 'postgres_table')
                )
                mappings.append(mapping)
        
        logger.info(f"Loaded {len(mappings)} mapping rules from {csv_path}")
        return mappings
        
    except Exception as e:
        logger.error(f"Failed to load mappings from {csv_path}: {e}")
        raise


def create_convention_based_mappings(pattern: str, entities: List[str]) -> List[OwnershipMapping]:
    """Create mappings based on naming conventions"""
    # Example conventions:
    # - postgres_table:hr_* -> owned by HR-Team group
    # - postgres_table:finance_* -> owned by Finance-Team group
    
    mappings = []
    
    # Extract department/team name from pattern
    # Pattern: "postgres_table:{dept}_*" -> dept becomes the owner group
    match = re.match(r'postgres_table:(\w+)_\*', pattern)
    if match:
        dept = match.group(1)
        owner_group = f"{dept.capitalize()}-Team"
        
        mapping = OwnershipMapping(
            entity_pattern=pattern,
            owner_group=owner_group,
            visible_to_groups=[f"{dept.capitalize()}-Readers", f"Data-Admins"],
            entity_type='postgres_table'
        )
        mappings.append(mapping)
        
        logger.info(f"Created convention-based mapping: {pattern} -> {owner_group}")
    
    return mappings


def load_entities_from_jsonl(jsonl_path: str, entity_types: List[str] = None) -> List[str]:
    """Load entity IDs from a JSONL file"""
    entity_types = entity_types or ['postgres_table', 'postgres_schema', 'postgres_view']
    entities = []
    
    try:
        with open(jsonl_path, 'r') as f:
            for line in f:
                if not line.strip():
                    continue
                
                entity = json.loads(line)
                entity_type = entity.get('type')
                entity_id = entity.get('id')
                
                if entity_type in entity_types and entity_id:
                    entities.append(entity_id)
        
        logger.info(f"Loaded {len(entities)} entities from {jsonl_path}")
        return entities
        
    except Exception as e:
        logger.error(f"Failed to load entities from {jsonl_path}: {e}")
        raise


def generate_sample_entities() -> List[str]:
    """Generate sample entity IDs for testing"""
    return [
        "postgres_table:public.users",
        "postgres_table:public.orders",
        "postgres_table:hr.employees",
        "postgres_table:hr.salaries",
        "postgres_table:finance.invoices",
        "postgres_table:finance.payments",
        "postgres_schema:public",
        "postgres_schema:hr",
        "postgres_schema:finance"
    ]


def main():
    parser = argparse.ArgumentParser(
        description='Create ownership and visibility edges between PostgreSQL entities and AD groups',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument(
        '--mapping',
        type=str,
        help='Path to mapping configuration file (YAML or CSV)'
    )
    parser.add_argument(
        '--convention',
        type=str,
        help='Naming convention pattern (e.g., "postgres_table:hr_*")'
    )
    parser.add_argument(
        '--entities',
        type=str,
        help='Path to JSONL file containing entities to link (optional, uses sample if not provided)'
    )
    parser.add_argument(
        '--output',
        type=str,
        default='ownership_edges.jsonl',
        help='Output JSONL file for edges (default: ownership_edges.jsonl)'
    )
    parser.add_argument(
        '--entity-types',
        type=str,
        nargs='+',
        default=['postgres_table', 'postgres_schema', 'postgres_view'],
        help='Entity types to process (default: postgres_table postgres_schema postgres_view)'
    )
    
    args = parser.parse_args()
    
    # Load mappings
    mappings = []
    
    if args.mapping:
        if args.mapping.endswith('.yaml') or args.mapping.endswith('.yml'):
            mappings = load_mappings_from_yaml(args.mapping)
        elif args.mapping.endswith('.csv'):
            mappings = load_mappings_from_csv(args.mapping)
        else:
            logger.error("Unsupported mapping file format. Use .yaml, .yml, or .csv")
            sys.exit(1)
    elif args.convention:
        # Load entities first to create convention-based mappings
        if args.entities:
            entities = load_entities_from_jsonl(args.entities, args.entity_types)
        else:
            entities = generate_sample_entities()
        
        mappings = create_convention_based_mappings(args.convention, entities)
    else:
        parser.error("Either --mapping or --convention must be specified")
    
    if not mappings:
        logger.error("No mappings loaded or created")
        sys.exit(1)
    
    # Load entities
    if args.entities:
        entities = load_entities_from_jsonl(args.entities, args.entity_types)
    else:
        logger.info("No entities file specified, using sample entities")
        entities = generate_sample_entities()
    
    if not entities:
        logger.error("No entities to process")
        sys.exit(1)
    
    # Create linker and process
    linker = OwnershipLinker(mappings)
    
    if not linker.link_all(entities, args.output):
        logger.error("Ownership linkage failed")
        sys.exit(1)
    
    logger.info("Ownership linkage completed successfully")


if __name__ == '__main__':
    main()
