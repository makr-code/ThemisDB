"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingest.py                                          ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     889                                            ║
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
ThemisDB Ingestion Tool
=======================

Ein Werkzeug zur rekursiven Durchsuchung von Verzeichnissen nach ingestierbaren
Dateien und deren Aufbereitung für ThemisDB. Unterstützt Metadatenextraktion für
Graph-, Vektor- und relationale Modelle.

Features:
- Rekursive Verzeichnisdurchsuchung
- JSON/YAML Metadatenunterstützung
- Hash-basierte Duplikaterkennung (SHA256)
- Fortschrittsanzeige mit detailliertem Logging
- Metadatenextraktion für ThemisDB-Modelle (Graph, Vector, Relational)
- Unterstützung verschiedener Dateitypen (JSON, YAML, Text, CSV, etc.)
- Integration mit ThemisDB BaseEntity Struktur
- Kompatibel mit ThemisDB Importer Interface

Verwendung:
    python3 tools/ingest.py --source /path/to/data --output ingestion_output.json
    python3 tools/ingest.py --source /path/to/data --config ingest_config.yaml
"""

import argparse
import hashlib
import json
import logging
import os
import sqlite3
import sys
import time
from collections import defaultdict
from dataclasses import dataclass, asdict
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Set, Optional, Any
import mimetypes
import csv

try:
    import yaml
except ImportError:
    yaml = None

try:
    from tqdm import tqdm
except ImportError:
    tqdm = None


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('ingestion.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('ThemisDB-Ingest')


@dataclass
class FileMetadata:
    """Metadata for an ingested file"""
    file_path: str
    file_hash: str
    file_size: int
    mime_type: str
    ingestion_time: str
    metadata: Dict[str, Any]
    themis_metadata: Dict[str, Any]


@dataclass
class IngestionConfig:
    """Configuration for ingestion process"""
    source_dir: str
    output_file: str = "ingestion_output.json"
    db_path: str = "ingestion_tracker.db"
    
    # File filters
    include_extensions: List[str] = None
    exclude_extensions: List[str] = None
    exclude_patterns: List[str] = None
    
    # Processing options
    max_file_size_mb: float = 100.0
    extract_text_preview: bool = True
    preview_length: int = 500
    
    # ThemisDB specific
    generate_vector_metadata: bool = True
    generate_graph_metadata: bool = True
    generate_relational_metadata: bool = True
    
    # Metadata extraction
    metadata_file_patterns: List[str] = None
    
    def __post_init__(self):
        if self.include_extensions is None:
            self.include_extensions = []
        if self.exclude_extensions is None:
            self.exclude_extensions = ['.exe', '.dll', '.so', '.dylib', '.bin']
        if self.exclude_patterns is None:
            self.exclude_patterns = ['.git', '__pycache__', 'node_modules', '.venv']
        if self.metadata_file_patterns is None:
            self.metadata_file_patterns = ['*.meta.json', '*.meta.yaml', '*.meta.yml']


class IngestionTracker:
    """SQLite-based tracker for ingested files"""
    
    def __init__(self, db_path: str):
        self.db_path = db_path
        self.conn = None
        self._initialize_db()
    
    def _initialize_db(self):
        """Initialize SQLite database"""
        self.conn = sqlite3.connect(self.db_path)
        cursor = self.conn.cursor()
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS ingested_files (
                file_path TEXT PRIMARY KEY,
                file_hash TEXT NOT NULL,
                file_size INTEGER,
                mime_type TEXT,
                ingestion_time TEXT,
                metadata TEXT,
                themis_metadata TEXT
            )
        ''')
        cursor.execute('''
            CREATE INDEX IF NOT EXISTS idx_file_hash ON ingested_files(file_hash)
        ''')
        self.conn.commit()
    
    def is_ingested(self, file_path: str, file_hash: str) -> bool:
        """Check if file with same hash has been ingested"""
        cursor = self.conn.cursor()
        cursor.execute(
            'SELECT file_hash FROM ingested_files WHERE file_path = ? OR file_hash = ?',
            (file_path, file_hash)
        )
        result = cursor.fetchone()
        return result is not None
    
    def add_file(self, file_meta: FileMetadata):
        """Add ingested file to tracker"""
        cursor = self.conn.cursor()
        cursor.execute('''
            INSERT OR REPLACE INTO ingested_files 
            (file_path, file_hash, file_size, mime_type, ingestion_time, metadata, themis_metadata)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (
            file_meta.file_path,
            file_meta.file_hash,
            file_meta.file_size,
            file_meta.mime_type,
            file_meta.ingestion_time,
            json.dumps(file_meta.metadata),
            json.dumps(file_meta.themis_metadata)
        ))
        self.conn.commit()
    
    def get_stats(self) -> Dict[str, int]:
        """Get ingestion statistics"""
        cursor = self.conn.cursor()
        cursor.execute('SELECT COUNT(*) FROM ingested_files')
        total_files = cursor.fetchone()[0]
        cursor.execute('SELECT SUM(file_size) FROM ingested_files')
        total_size = cursor.fetchone()[0] or 0
        return {
            'total_files': total_files,
            'total_size_bytes': total_size,
            'total_size_mb': total_size / (1024 * 1024)
        }
    
    def close(self):
        """Close database connection"""
        if self.conn:
            self.conn.close()


class FileProcessor:
    """Base class for file processors"""
    
    def __init__(self, config: IngestionConfig):
        self.config = config
    
    def compute_hash(self, file_path: str) -> str:
        """Compute SHA256 hash of file"""
        sha256_hash = hashlib.sha256()
        try:
            with open(file_path, "rb") as f:
                for byte_block in iter(lambda: f.read(4096), b""):
                    sha256_hash.update(byte_block)
            return sha256_hash.hexdigest()
        except Exception as e:
            logger.error(f"Error computing hash for {file_path}: {e}")
            return ""
    
    def get_mime_type(self, file_path: str) -> str:
        """Get MIME type of file"""
        mime_type, _ = mimetypes.guess_type(file_path)
        return mime_type or "application/octet-stream"
    
    def extract_metadata(self, file_path: str) -> Dict[str, Any]:
        """Extract generic metadata from file"""
        metadata = {
            'file_name': os.path.basename(file_path),
            'file_extension': os.path.splitext(file_path)[1],
            'created_time': datetime.fromtimestamp(os.path.getctime(file_path)).isoformat(),
            'modified_time': datetime.fromtimestamp(os.path.getmtime(file_path)).isoformat(),
        }
        return metadata
    
    def extract_themis_metadata(self, file_path: str, content: Any) -> Dict[str, Any]:
        """Extract ThemisDB-specific metadata"""
        themis_meta = {}
        
        # Graph metadata
        if self.config.generate_graph_metadata:
            themis_meta['graph'] = self._extract_graph_metadata(file_path, content)
        
        # Vector metadata
        if self.config.generate_vector_metadata:
            themis_meta['vector'] = self._extract_vector_metadata(file_path, content)
        
        # Relational metadata
        if self.config.generate_relational_metadata:
            themis_meta['relational'] = self._extract_relational_metadata(file_path, content)
        
        # Geo/Spatial metadata
        geo_meta = self._extract_geo_metadata(file_path, content)
        if geo_meta:
            themis_meta['geo'] = geo_meta
        
        # Process metadata
        process_meta = self._extract_process_metadata(file_path, content)
        if process_meta:
            themis_meta['process'] = process_meta
        
        return themis_meta
    
    def _extract_graph_metadata(self, file_path: str, content: Any) -> Dict[str, Any]:
        """Extract graph model metadata"""
        graph_meta = {
            'entity_type': 'Document',
            'entity_id': os.path.basename(file_path),
            'properties': {
                'source_file': file_path,
                'file_type': os.path.splitext(file_path)[1]
            },
            'relationships': []
        }
        
        # Check for potential relationships based on content
        if isinstance(content, dict):
            # Look for common relationship indicators
            for key in ['references', 'links', 'related', 'dependencies']:
                if key in content:
                    graph_meta['relationships'].append({
                        'type': key,
                        'targets': content[key] if isinstance(content[key], list) else [content[key]]
                    })
        
        return graph_meta
    
    def _extract_vector_metadata(self, file_path: str, content: Any) -> Dict[str, Any]:
        """Extract vector model metadata"""
        vector_meta = {
            'object_name': 'documents',
            'document_id': os.path.basename(file_path),
            'content_type': self.get_mime_type(file_path),
            'embedding_required': True
        }
        
        # Extract text for embedding
        if isinstance(content, str):
            preview = content[:self.config.preview_length] if self.config.extract_text_preview else content
            vector_meta['text_content'] = preview
            vector_meta['content_length'] = len(content)
        elif isinstance(content, dict):
            # Convert dict to searchable text
            text_parts = []
            for key, value in content.items():
                if isinstance(value, (str, int, float, bool)):
                    text_parts.append(f"{key}: {value}")
            text_content = " ".join(text_parts)
            vector_meta['text_content'] = text_content[:self.config.preview_length]
            vector_meta['content_length'] = len(text_content)
        
        return vector_meta
    
    def _extract_relational_metadata(self, file_path: str, content: Any) -> Dict[str, Any]:
        """Extract relational model metadata"""
        relational_meta = {
            'table_name': 'ingested_documents',
            'schema': {
                'id': 'TEXT PRIMARY KEY',
                'file_path': 'TEXT NOT NULL',
                'file_name': 'TEXT',
                'content_type': 'TEXT',
                'ingestion_date': 'TIMESTAMP'
            },
            'record': {
                'id': os.path.basename(file_path),
                'file_path': file_path,
                'file_name': os.path.basename(file_path),
                'content_type': self.get_mime_type(file_path),
                'ingestion_date': datetime.now().isoformat()
            }
        }
        
        # Add content-specific fields for structured data
        if isinstance(content, dict):
            for key, value in content.items():
                if isinstance(value, (str, int, float, bool)):
                    # Add to schema and record
                    field_type = 'TEXT'
                    if isinstance(value, int):
                        field_type = 'INTEGER'
                    elif isinstance(value, float):
                        field_type = 'REAL'
                    elif isinstance(value, bool):
                        field_type = 'BOOLEAN'
                    
                    relational_meta['schema'][key] = field_type
                    relational_meta['record'][key] = value
        
        return relational_meta
    
    def _extract_geo_metadata(self, file_path: str, content: Any) -> Optional[Dict[str, Any]]:
        """Extract geo/spatial metadata"""
        if not isinstance(content, dict):
            return None
        
        geo_meta = {}
        
        # Look for common geo fields
        geo_fields = {
            'latitude': ['latitude', 'lat', 'y', 'coord_lat'],
            'longitude': ['longitude', 'lon', 'lng', 'x', 'coord_lon', 'coord_lng'],
            'address': ['address', 'addr', 'street_address', 'location'],
            'city': ['city', 'town', 'municipality'],
            'postal_code': ['postal_code', 'zip', 'zipcode', 'plz', 'postcode'],
            'country': ['country', 'nation', 'land'],
            'geometry': ['geometry', '_geometry', 'geom', 'shape'],
            'coordinates': ['coordinates', 'coords', 'point']
        }
        
        found_geo = False
        for field_type, field_names in geo_fields.items():
            for key, value in content.items():
                if key.lower() in field_names:
                    geo_meta[field_type] = value
                    found_geo = True
        
        if not found_geo:
            return None
        
        # Build geo metadata
        result = {
            'has_geometry': False,
            'coordinate_fields': {},
            'address_fields': {}
        }
        
        # Extract coordinates
        if 'latitude' in geo_meta and 'longitude' in geo_meta:
            try:
                lat = float(geo_meta['latitude'])
                lon = float(geo_meta['longitude'])
                result['has_geometry'] = True
                result['coordinate_fields'] = {
                    'latitude': lat,
                    'longitude': lon,
                    'srid': 4326  # WGS84
                }
                # Generate WKT Point
                result['geometry_wkt'] = f"POINT({lon} {lat})"
            except (ValueError, TypeError):
                pass
        
        # Extract geometry field (WKT, GeoJSON, EWKB)
        if 'geometry' in geo_meta:
            geom = geo_meta['geometry']
            if isinstance(geom, str):
                result['has_geometry'] = True
                result['geometry_raw'] = geom
                # Try to determine format
                if geom.upper().startswith(('POINT', 'LINESTRING', 'POLYGON', 'MULTIPOINT', 'MULTILINESTRING', 'MULTIPOLYGON')):
                    result['geometry_format'] = 'WKT'
                elif geom.startswith('{') and 'type' in geom:
                    result['geometry_format'] = 'GeoJSON'
            elif isinstance(geom, dict):
                result['has_geometry'] = True
                result['geometry_raw'] = geom
                result['geometry_format'] = 'GeoJSON'
        
        # Extract coordinates array [lon, lat] or [[lon, lat], ...]
        # Only if we don't already have coordinates from lat/lon fields
        if 'coordinates' in geo_meta and not result.get('coordinate_fields'):
            coords = geo_meta['coordinates']
            if isinstance(coords, (list, tuple)) and len(coords) >= 2:
                try:
                    if isinstance(coords[0], (int, float)):
                        # Single point [lon, lat]
                        lon, lat = float(coords[0]), float(coords[1])
                        result['has_geometry'] = True
                        result['coordinate_fields'] = {
                            'longitude': lon,
                            'latitude': lat,
                            'srid': 4326
                        }
                        result['geometry_wkt'] = f"POINT({lon} {lat})"
                except (ValueError, TypeError, IndexError):
                    pass
        
        # Extract address information
        address_components = {}
        for field in ['address', 'city', 'postal_code', 'country']:
            if field in geo_meta:
                address_components[field] = geo_meta[field]
        
        if address_components:
            result['address_fields'] = address_components
            # Generate full address string
            address_parts = []
            if 'address' in address_components:
                address_parts.append(str(address_components['address']))
            if 'postal_code' in address_components:
                address_parts.append(str(address_components['postal_code']))
            if 'city' in address_components:
                address_parts.append(str(address_components['city']))
            if 'country' in address_components:
                address_parts.append(str(address_components['country']))
            result['full_address'] = ', '.join(address_parts)
        
        # Add spatial index hint
        if result.get('has_geometry'):
            result['spatial_index_required'] = True
            result['index_type'] = 'R-Tree'
        
        return result if (result.get('has_geometry') or result.get('address_fields')) else None
    
    def _extract_process_metadata(self, file_path: str, content: Any) -> Optional[Dict[str, Any]]:
        """Extract process-aware metadata (BPMN, workflow, state machine)"""
        if not isinstance(content, dict):
            return None
        
        process_meta = {}
        
        # Check for process-related fields
        process_indicators = {
            'state': ['state', 'status', '_state', 'current_state', 'process_state'],
            'activity': ['activity', 'task', 'action', 'step', 'phase'],
            'case_id': ['case_id', 'process_id', 'instance_id', 'workflow_id'],
            'timestamp': ['timestamp', 'time', 'date', 'created_at', 'updated_at'],
            'resource': ['resource', 'user', 'actor', 'assignee', 'owner'],
            'variables': ['variables', '_variables', 'data', 'context'],
            'tokens': ['tokens', '_tokens', 'positions'],
            'transitions': ['transitions', 'edges', 'flows'],
            'process_type': ['type', '_type', 'process_type', 'workflow_type']
        }
        
        found_process = False
        for field_type, field_names in process_indicators.items():
            for key, value in content.items():
                if key.lower() in field_names:
                    process_meta[field_type] = value
                    found_process = True
        
        if not found_process:
            return None
        
        # Build process metadata
        result = {
            'is_process_aware': True,
            'process_fields': {}
        }
        
        # Extract state information
        if 'state' in process_meta:
            result['process_fields']['state'] = process_meta['state']
            result['has_state'] = True
        
        # Extract activity/task information
        if 'activity' in process_meta:
            result['process_fields']['activity'] = process_meta['activity']
        
        # Extract case/instance ID
        if 'case_id' in process_meta:
            result['process_fields']['case_id'] = process_meta['case_id']
            result['is_process_instance'] = True
        
        # Extract timestamp
        if 'timestamp' in process_meta:
            result['process_fields']['timestamp'] = process_meta['timestamp']
        
        # Extract resource/actor
        if 'resource' in process_meta:
            result['process_fields']['resource'] = process_meta['resource']
        
        # Extract process variables
        if 'variables' in process_meta:
            result['process_fields']['variables'] = process_meta['variables']
            result['has_variables'] = True
        
        # Extract tokens (for Petri nets / process execution)
        if 'tokens' in process_meta:
            result['process_fields']['tokens'] = process_meta['tokens']
            result['has_tokens'] = True
        
        # Extract process type
        if 'process_type' in process_meta:
            result['process_type'] = process_meta['process_type']
        
        # Check for BPMN-specific fields
        bpmn_fields = ['bpmn', 'flowNode', 'sequenceFlow', 'gateway', 'event', 'task']
        if any(key in content for key in bpmn_fields):
            result['format'] = 'BPMN'
            result['is_bpmn'] = True
        
        # Check for state machine fields
        if 'transitions' in process_meta:
            result['transitions'] = process_meta['transitions']
            result['is_state_machine'] = True
        
        # Add process mining hints
        result['process_mining_ready'] = bool(
            'case_id' in process_meta and 
            'activity' in process_meta and 
            'timestamp' in process_meta
        )
        
        # Suggest collection names for process storage
        if result.get('is_process_instance'):
            result['suggested_collection'] = '_process_instances'
        elif result.get('is_bpmn'):
            result['suggested_collection'] = '_process_definitions'
        
        return result
    
    def process_file(self, file_path: str) -> Optional[FileMetadata]:
        """Process a single file and extract metadata"""
        try:
            # Compute hash
            file_hash = self.compute_hash(file_path)
            if not file_hash:
                return None
            
            # Get basic file info
            file_size = os.path.getsize(file_path)
            mime_type = self.get_mime_type(file_path)
            
            # Extract metadata
            metadata = self.extract_metadata(file_path)
            
            # Load content based on file type
            content = self._load_content(file_path)
            
            # Extract ThemisDB metadata
            themis_metadata = self.extract_themis_metadata(file_path, content)
            
            # Create FileMetadata object
            file_meta = FileMetadata(
                file_path=file_path,
                file_hash=file_hash,
                file_size=file_size,
                mime_type=mime_type,
                ingestion_time=datetime.now().isoformat(),
                metadata=metadata,
                themis_metadata=themis_metadata
            )
            
            return file_meta
            
        except Exception as e:
            logger.error(f"Error processing file {file_path}: {e}")
            return None
    
    def _load_content(self, file_path: str) -> Any:
        """Load file content based on type"""
        ext = os.path.splitext(file_path)[1].lower()
        
        try:
            if ext == '.json':
                with open(file_path, 'r', encoding='utf-8') as f:
                    return json.load(f)
            elif ext in ['.yaml', '.yml'] and yaml:
                with open(file_path, 'r', encoding='utf-8') as f:
                    return yaml.safe_load(f)
            elif ext == '.csv':
                with open(file_path, 'r', encoding='utf-8') as f:
                    reader = csv.DictReader(f)
                    return list(reader)
            elif ext in ['.txt', '.md', '.log']:
                with open(file_path, 'r', encoding='utf-8') as f:
                    return f.read()
            else:
                # For binary or unknown files, return None
                return None
        except Exception as e:
            logger.warning(f"Could not load content from {file_path}: {e}")
            return None


class IngestionEngine:
    """Main ingestion engine"""
    
    def __init__(self, config: IngestionConfig):
        self.config = config
        self.tracker = IngestionTracker(config.db_path)
        self.processor = FileProcessor(config)
        self.stats = {
            'total_files_scanned': 0,
            'files_processed': 0,
            'files_skipped': 0,
            'files_failed': 0,
            'total_size_bytes': 0
        }
    
    def should_process_file(self, file_path: str) -> bool:
        """Determine if file should be processed"""
        # Check exclude patterns
        for pattern in self.config.exclude_patterns:
            if pattern in file_path:
                return False
        
        # Check file extension
        ext = os.path.splitext(file_path)[1].lower()
        
        if self.config.include_extensions:
            if ext not in self.config.include_extensions:
                return False
        
        if ext in self.config.exclude_extensions:
            return False
        
        # Check file size
        try:
            file_size = os.path.getsize(file_path)
            max_size = self.config.max_file_size_mb * 1024 * 1024
            if file_size > max_size:
                logger.warning(f"Skipping {file_path}: file too large ({file_size / 1024 / 1024:.2f} MB)")
                return False
        except Exception as e:
            logger.error(f"Error checking file size for {file_path}: {e}")
            return False
        
        return True
    
    def scan_directory(self, directory: str) -> List[str]:
        """Recursively scan directory for files"""
        files = []
        logger.info(f"Scanning directory: {directory}")
        
        for root, dirs, filenames in os.walk(directory):
            # Filter out excluded directories
            dirs[:] = [d for d in dirs if not any(pattern in d for pattern in self.config.exclude_patterns)]
            
            for filename in filenames:
                file_path = os.path.join(root, filename)
                if self.should_process_file(file_path):
                    files.append(file_path)
        
        logger.info(f"Found {len(files)} files to process")
        return files
    
    def ingest(self) -> Dict[str, Any]:
        """Run ingestion process"""
        logger.info("Starting ingestion process")
        start_time = time.time()
        
        # Scan directory
        files = self.scan_directory(self.config.source_dir)
        self.stats['total_files_scanned'] = len(files)
        
        # Process files
        ingested_files = []
        
        # Use tqdm if available, otherwise simple counter
        if tqdm:
            file_iterator = tqdm(files, desc="Ingesting files", unit="file")
        else:
            file_iterator = files
            logger.info(f"Processing {len(files)} files...")
        
        for i, file_path in enumerate(file_iterator):
            if not tqdm and (i + 1) % 10 == 0:
                logger.info(f"Progress: {i + 1}/{len(files)} files")
            
            # Compute hash first to check if already ingested
            file_hash = self.processor.compute_hash(file_path)
            if not file_hash:
                self.stats['files_failed'] += 1
                continue
            
            # Check if already ingested
            if self.tracker.is_ingested(file_path, file_hash):
                self.stats['files_skipped'] += 1
                if tqdm:
                    file_iterator.set_postfix({'skipped': self.stats['files_skipped']})
                continue
            
            # Process file
            file_meta = self.processor.process_file(file_path)
            if file_meta:
                self.tracker.add_file(file_meta)
                ingested_files.append(asdict(file_meta))
                self.stats['files_processed'] += 1
                self.stats['total_size_bytes'] += file_meta.file_size
            else:
                self.stats['files_failed'] += 1
        
        # Calculate elapsed time
        elapsed_time = time.time() - start_time
        self.stats['elapsed_seconds'] = elapsed_time
        
        # Add tracker stats
        tracker_stats = self.tracker.get_stats()
        self.stats.update(tracker_stats)
        
        # Log summary
        logger.info("Ingestion complete!")
        logger.info(f"Files scanned: {self.stats['total_files_scanned']}")
        logger.info(f"Files processed: {self.stats['files_processed']}")
        logger.info(f"Files skipped (already ingested): {self.stats['files_skipped']}")
        logger.info(f"Files failed: {self.stats['files_failed']}")
        logger.info(f"Total size: {self.stats['total_size_bytes'] / 1024 / 1024:.2f} MB")
        logger.info(f"Elapsed time: {elapsed_time:.2f} seconds")
        
        # Save output
        output = {
            'metadata': {
                'ingestion_time': datetime.now().isoformat(),
                'source_directory': self.config.source_dir,
                'config': asdict(self.config)
            },
            'statistics': self.stats,
            'ingested_files': ingested_files
        }
        
        with open(self.config.output_file, 'w') as f:
            json.dump(output, f, indent=2)
        
        logger.info(f"Output saved to: {self.config.output_file}")
        
        return output


def load_config_file(config_path: str) -> Dict[str, Any]:
    """Load configuration from YAML or JSON file"""
    ext = os.path.splitext(config_path)[1].lower()
    
    with open(config_path, 'r') as f:
        if ext == '.json':
            return json.load(f)
        elif ext in ['.yaml', '.yml']:
            if not yaml:
                raise ImportError("PyYAML is required for YAML config files. Install with: pip install pyyaml")
            return yaml.safe_load(f)
        else:
            raise ValueError(f"Unsupported config file format: {ext}")


def main():
    parser = argparse.ArgumentParser(
        description='ThemisDB Ingestion Tool - Recursively scan and ingest files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Basic ingestion
  python3 tools/ingest.py --source /path/to/data
  
  # With custom output file
  python3 tools/ingest.py --source /path/to/data --output results.json
  
  # Using config file
  python3 tools/ingest.py --config ingest_config.yaml
  
  # Filter by extensions
  python3 tools/ingest.py --source /path/to/data --include-ext .json .yaml .txt
        """
    )
    
    parser.add_argument('--source', type=str, help='Source directory to scan')
    parser.add_argument('--output', type=str, default='ingestion_output.json',
                        help='Output file for ingestion results (default: ingestion_output.json)')
    parser.add_argument('--config', type=str, help='Configuration file (YAML or JSON)')
    parser.add_argument('--db', type=str, default='ingestion_tracker.db',
                        help='SQLite database for tracking ingested files (default: ingestion_tracker.db)')
    
    # File filtering
    parser.add_argument('--include-ext', nargs='+', help='Include only these file extensions (e.g., .json .yaml)')
    parser.add_argument('--exclude-ext', nargs='+', help='Exclude these file extensions')
    parser.add_argument('--max-size', type=float, default=100.0,
                        help='Maximum file size in MB (default: 100.0)')
    
    # Features
    parser.add_argument('--no-vector', action='store_true', help='Disable vector metadata generation')
    parser.add_argument('--no-graph', action='store_true', help='Disable graph metadata generation')
    parser.add_argument('--no-relational', action='store_true', help='Disable relational metadata generation')
    
    parser.add_argument('--verbose', action='store_true', help='Enable verbose logging')
    
    args = parser.parse_args()
    
    # Set logging level
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    
    # Load configuration
    if args.config:
        try:
            config_dict = load_config_file(args.config)
            config = IngestionConfig(**config_dict)
        except Exception as e:
            logger.error(f"Error loading config file: {e}")
            return 1
    else:
        if not args.source:
            parser.print_help()
            logger.error("Either --source or --config must be specified")
            return 1
        
        config = IngestionConfig(
            source_dir=args.source,
            output_file=args.output,
            db_path=args.db,
            include_extensions=args.include_ext or [],
            exclude_extensions=args.exclude_ext or ['.exe', '.dll', '.so', '.dylib', '.bin'],
            max_file_size_mb=args.max_size,
            generate_vector_metadata=not args.no_vector,
            generate_graph_metadata=not args.no_graph,
            generate_relational_metadata=not args.no_relational
        )
    
    # Validate source directory
    if not os.path.isdir(config.source_dir):
        logger.error(f"Source directory does not exist: {config.source_dir}")
        return 1
    
    # Run ingestion
    try:
        engine = IngestionEngine(config)
        result = engine.ingest()
        logger.info("Ingestion completed successfully!")
        return 0
    except KeyboardInterrupt:
        logger.warning("Ingestion interrupted by user")
        return 130
    except Exception as e:
        logger.error(f"Ingestion failed: {e}", exc_info=True)
        return 1
    finally:
        # Close tracker
        if 'engine' in locals():
            engine.tracker.close()


if __name__ == '__main__':
    sys.exit(main())
