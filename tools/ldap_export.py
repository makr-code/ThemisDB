"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ldap_export.py                                     ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:24:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     714                                            ║
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
ThemisDB LDAP/Active Directory Exporter
=========================================

Exports Active Directory/LDAP directory objects (Users, Groups, OUs) to JSONL format
for ingestion into ThemisDB. Supports pagination, filtering, and configurable attribute extraction.

Features:
- LDAP connection with bind authentication
- Paged searching for large directories
- Exports Users, Groups, and Organizational Units
- Configurable attribute mapping
- JSONL output compatible with tools/ingest.py
- Graph structure: nodes (ad_user, ad_group, ad_ou) and edges (MEMBER_OF, CHILD_OF, IN_OU)
- Optional file-link style keys for simple aliasing

Usage:
    python3 tools/ldap_export.py --config ldap_export_config.yaml --output ad_export.jsonl
    python3 tools/ldap_export.py --server ldap://dc.example.com --base-dn "DC=example,DC=com" --output ad_export.jsonl
"""

import argparse
import hashlib
import json
import logging
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import List, Dict, Set, Optional, Any

try:
    import yaml
except ImportError:
    yaml = None

try:
    import ldap3
    from ldap3 import Server, Connection, ALL, SUBTREE
    from ldap3.core.exceptions import LDAPException
except ImportError:
    ldap3 = None
    print("ERROR: ldap3 library not found. Install with: pip install ldap3", file=sys.stderr)
    sys.exit(1)


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('ldap_export.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('ThemisDB-LDAP-Export')


class LDAPExportConfig:
    """Configuration for LDAP export"""
    def __init__(self, config_dict: Dict[str, Any] = None):
        config_dict = config_dict or {}
        
        # LDAP Connection
        self.server = config_dict.get('server', 'ldap://localhost')
        self.port = config_dict.get('port', 389)
        self.use_ssl = config_dict.get('use_ssl', False)
        self.bind_dn = config_dict.get('bind_dn', '')
        self.bind_password = config_dict.get('bind_password', '')
        self.base_dn = config_dict.get('base_dn', 'DC=example,DC=com')
        
        # Search Configuration
        self.user_filter = config_dict.get('user_filter', '(objectClass=user)')
        self.group_filter = config_dict.get('group_filter', '(objectClass=group)')
        self.ou_filter = config_dict.get('ou_filter', '(objectClass=organizationalUnit)')
        self.page_size = config_dict.get('page_size', 1000)
        
        # Attributes to export
        self.user_attributes = config_dict.get('user_attributes', [
            'sAMAccountName', 'objectGUID', 'distinguishedName', 'mail',
            'displayName', 'memberOf', 'userPrincipalName', 'cn', 'description'
        ])
        self.group_attributes = config_dict.get('group_attributes', [
            'sAMAccountName', 'objectGUID', 'distinguishedName', 'cn',
            'description', 'member', 'memberOf'
        ])
        self.ou_attributes = config_dict.get('ou_attributes', [
            'objectGUID', 'distinguishedName', 'ou', 'description'
        ])
        
        # Output Configuration
        self.output_file = config_dict.get('output_file', 'ad_export.jsonl')
        self.generate_file_links = config_dict.get('generate_file_links', False)
        self.file_link_prefix = config_dict.get('file_link_prefix', 'file:ad/')
        
        # Processing Options
        self.export_users = config_dict.get('export_users', True)
        self.export_groups = config_dict.get('export_groups', True)
        self.export_ous = config_dict.get('export_ous', True)
        self.max_entries = config_dict.get('max_entries', None)  # None = no limit


class LDAPExporter:
    """Exports LDAP/AD objects to JSONL format for ThemisDB ingestion"""
    
    def __init__(self, config: LDAPExportConfig):
        self.config = config
        self.connection = None
        self.exported_count = {
            'users': 0,
            'groups': 0,
            'ous': 0,
            'edges': 0
        }
        self.guid_to_dn = {}  # Map objectGUID to DN for edge creation
        
    def connect(self) -> bool:
        """Connect to LDAP server"""
        try:
            logger.info(f"Connecting to LDAP server: {self.config.server}")
            
            server = Server(
                self.config.server,
                port=self.config.port,
                use_ssl=self.config.use_ssl,
                get_info=ALL
            )
            
            self.connection = Connection(
                server,
                user=self.config.bind_dn,
                password=self.config.bind_password,
                auto_bind=True
            )
            
            logger.info(f"Successfully connected to {self.config.server}")
            logger.info(f"Server info: {server.info}")
            return True
            
        except LDAPException as e:
            logger.error(f"Failed to connect to LDAP server: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from LDAP server"""
        if self.connection:
            self.connection.unbind()
            logger.info("Disconnected from LDAP server")
    
    def _guid_to_string(self, guid_bytes) -> str:
        """Convert binary GUID to string format"""
        if isinstance(guid_bytes, bytes):
            # Convert bytes to UUID string
            guid = uuid.UUID(bytes_le=guid_bytes)
            return str(guid)
        return str(guid_bytes)
    
    def _extract_ou_from_dn(self, dn: str) -> List[str]:
        """Extract OU path from distinguished name"""
        parts = dn.split(',')
        ous = [part.split('=')[1] for part in parts if part.strip().upper().startswith('OU=')]
        return ous
    
    def _create_node_entity(self, node_type: str, guid: str, attributes: Dict[str, Any]) -> Dict[str, Any]:
        """Create a ThemisDB node entity"""
        entity_id = f"{node_type}:{guid}"
        
        entity = {
            "id": entity_id,
            "type": node_type,
            "attributes": attributes,
            "metadata": {
                "source": "ldap_export",
                "export_time": datetime.now(timezone.utc).isoformat(),
                "guid": guid
            }
        }
        
        # Optional: Generate file-link style key
        if self.config.generate_file_links:
            entity["file_link"] = f"{self.config.file_link_prefix}{guid}.json"
        
        return entity
    
    def _create_edge_entity(self, edge_type: str, source_id: str, target_id: str, attributes: Dict[str, Any] = None) -> Dict[str, Any]:
        """Create a ThemisDB edge entity"""
        edge_id = f"edge:{hashlib.sha256(f'{source_id}-{edge_type}-{target_id}'.encode()).hexdigest()[:16]}"
        
        edge = {
            "id": edge_id,
            "type": edge_type,
            "source": source_id,
            "target": target_id,
            "attributes": attributes or {},
            "metadata": {
                "source": "ldap_export",
                "export_time": datetime.now(timezone.utc).isoformat()
            }
        }
        
        return edge
    
    def _search_paged(self, search_filter: str, attributes: List[str], object_type: str) -> List[Dict[str, Any]]:
        """Perform paged LDAP search"""
        results = []
        
        try:
            logger.info(f"Searching for {object_type} with filter: {search_filter}")
            
            self.connection.search(
                search_base=self.config.base_dn,
                search_filter=search_filter,
                search_scope=SUBTREE,
                attributes=attributes,
                paged_size=self.config.page_size
            )
            
            total_entries = 0
            while True:
                for entry in self.connection.entries:
                    if self.config.max_entries and total_entries >= self.config.max_entries:
                        logger.info(f"Reached max entries limit: {self.config.max_entries}")
                        return results
                    
                    entry_dict = json.loads(entry.entry_to_json())
                    results.append(entry_dict)
                    total_entries += 1
                
                # Get next page
                cookie = self.connection.result['controls']['1.2.840.113556.1.4.319']['value']['cookie']
                if not cookie:
                    break
                
                self.connection.search(
                    search_base=self.config.base_dn,
                    search_filter=search_filter,
                    search_scope=SUBTREE,
                    attributes=attributes,
                    paged_size=self.config.page_size,
                    paged_cookie=cookie
                )
            
            logger.info(f"Found {len(results)} {object_type} entries")
            return results
            
        except LDAPException as e:
            logger.error(f"LDAP search failed for {object_type}: {e}")
            return []
    
    def export_users(self, output_file) -> int:
        """Export AD users"""
        if not self.config.export_users:
            return 0
        
        logger.info("Exporting users...")
        users = self._search_paged(
            self.config.user_filter,
            self.config.user_attributes,
            'users'
        )
        
        count = 0
        for user in users:
            attrs = user.get('attributes', {})
            
            # Extract GUID
            guid_raw = attrs.get('objectGUID', [None])[0]
            if not guid_raw:
                logger.warning(f"User without objectGUID: {attrs.get('distinguishedName')}")
                continue
            
            guid = self._guid_to_string(guid_raw)
            dn = attrs.get('distinguishedName', [None])[0]
            
            # Store GUID to DN mapping
            if dn:
                self.guid_to_dn[guid] = dn
            
            # Create node attributes
            node_attrs = {
                'sAMAccountName': attrs.get('sAMAccountName', [None])[0],
                'mail': attrs.get('mail', [None])[0],
                'displayName': attrs.get('displayName', [None])[0],
                'userPrincipalName': attrs.get('userPrincipalName', [None])[0],
                'cn': attrs.get('cn', [None])[0],
                'description': attrs.get('description', [None])[0],
                'distinguishedName': dn
            }
            
            # Extract OU path
            if dn:
                node_attrs['ou_path'] = self._extract_ou_from_dn(dn)
            
            # Create node entity
            entity = self._create_node_entity('ad_user', guid, node_attrs)
            output_file.write(json.dumps(entity) + '\n')
            count += 1
            
            # Create MEMBER_OF edges
            member_of = attrs.get('memberOf', [])
            for group_dn in member_of:
                # We'll create edges after all nodes are exported
                # Store for later processing
                pass
        
        self.exported_count['users'] = count
        logger.info(f"Exported {count} users")
        return count
    
    def export_groups(self, output_file) -> int:
        """Export AD groups"""
        if not self.config.export_groups:
            return 0
        
        logger.info("Exporting groups...")
        groups = self._search_paged(
            self.config.group_filter,
            self.config.group_attributes,
            'groups'
        )
        
        count = 0
        for group in groups:
            attrs = group.get('attributes', {})
            
            # Extract GUID
            guid_raw = attrs.get('objectGUID', [None])[0]
            if not guid_raw:
                logger.warning(f"Group without objectGUID: {attrs.get('distinguishedName')}")
                continue
            
            guid = self._guid_to_string(guid_raw)
            dn = attrs.get('distinguishedName', [None])[0]
            
            # Store GUID to DN mapping
            if dn:
                self.guid_to_dn[guid] = dn
            
            # Create node attributes
            node_attrs = {
                'sAMAccountName': attrs.get('sAMAccountName', [None])[0],
                'cn': attrs.get('cn', [None])[0],
                'description': attrs.get('description', [None])[0],
                'distinguishedName': dn
            }
            
            # Extract OU path
            if dn:
                node_attrs['ou_path'] = self._extract_ou_from_dn(dn)
            
            # Create node entity
            entity = self._create_node_entity('ad_group', guid, node_attrs)
            output_file.write(json.dumps(entity) + '\n')
            count += 1
        
        self.exported_count['groups'] = count
        logger.info(f"Exported {count} groups")
        return count
    
    def export_ous(self, output_file) -> int:
        """Export Organizational Units"""
        if not self.config.export_ous:
            return 0
        
        logger.info("Exporting organizational units...")
        ous = self._search_paged(
            self.config.ou_filter,
            self.config.ou_attributes,
            'ous'
        )
        
        count = 0
        for ou in ous:
            attrs = ou.get('attributes', {})
            
            # Extract GUID
            guid_raw = attrs.get('objectGUID', [None])[0]
            if not guid_raw:
                logger.warning(f"OU without objectGUID: {attrs.get('distinguishedName')}")
                continue
            
            guid = self._guid_to_string(guid_raw)
            dn = attrs.get('distinguishedName', [None])[0]
            
            # Store GUID to DN mapping
            if dn:
                self.guid_to_dn[guid] = dn
            
            # Create node attributes
            node_attrs = {
                'ou': attrs.get('ou', [None])[0],
                'description': attrs.get('description', [None])[0],
                'distinguishedName': dn
            }
            
            # Create node entity
            entity = self._create_node_entity('ad_ou', guid, node_attrs)
            output_file.write(json.dumps(entity) + '\n')
            count += 1
        
        self.exported_count['ous'] = count
        logger.info(f"Exported {count} organizational units")
        return count
    
    def export_edges(self, output_file) -> int:
        """Export relationships (edges) between entities"""
        logger.info("Exporting edges...")
        
        # Re-query to get membership relationships
        # This is a second pass to create edges now that we have all GUIDs
        
        count = 0
        dn_to_guid = {v: k for k, v in self.guid_to_dn.items()}
        
        # Export user membership edges
        if self.config.export_users:
            users = self._search_paged(
                self.config.user_filter,
                ['objectGUID', 'memberOf', 'distinguishedName'],
                'user_edges'
            )
            
            for user in users:
                attrs = user.get('attributes', {})
                user_guid_raw = attrs.get('objectGUID', [None])[0]
                if not user_guid_raw:
                    continue
                
                user_guid = self._guid_to_string(user_guid_raw)
                user_dn = attrs.get('distinguishedName', [None])[0]
                member_of = attrs.get('memberOf', [])
                
                for group_dn in member_of:
                    group_guid = dn_to_guid.get(group_dn)
                    if group_guid:
                        edge = self._create_edge_entity(
                            'MEMBER_OF',
                            f'ad_user:{user_guid}',
                            f'ad_group:{group_guid}'
                        )
                        output_file.write(json.dumps(edge) + '\n')
                        count += 1
                
                # Create IN_OU edge
                ou_path = self._extract_ou_from_dn(user_dn) if user_dn else []
                if ou_path:
                    # Link to immediate parent OU
                    parent_ou_dn = ','.join([f'OU={ou}' for ou in ou_path[:1]] + user_dn.split(',')[1:])
                    parent_ou_guid = dn_to_guid.get(parent_ou_dn)
                    if parent_ou_guid:
                        edge = self._create_edge_entity(
                            'IN_OU',
                            f'ad_user:{user_guid}',
                            f'ad_ou:{parent_ou_guid}'
                        )
                        output_file.write(json.dumps(edge) + '\n')
                        count += 1
        
        # Export group membership edges
        if self.config.export_groups:
            groups = self._search_paged(
                self.config.group_filter,
                ['objectGUID', 'memberOf', 'distinguishedName'],
                'group_edges'
            )
            
            for group in groups:
                attrs = group.get('attributes', {})
                group_guid_raw = attrs.get('objectGUID', [None])[0]
                if not group_guid_raw:
                    continue
                
                group_guid = self._guid_to_string(group_guid_raw)
                group_dn = attrs.get('distinguishedName', [None])[0]
                member_of = attrs.get('memberOf', [])
                
                # Groups can be members of other groups
                for parent_group_dn in member_of:
                    parent_group_guid = dn_to_guid.get(parent_group_dn)
                    if parent_group_guid:
                        edge = self._create_edge_entity(
                            'MEMBER_OF',
                            f'ad_group:{group_guid}',
                            f'ad_group:{parent_group_guid}'
                        )
                        output_file.write(json.dumps(edge) + '\n')
                        count += 1
                
                # Create IN_OU edge
                ou_path = self._extract_ou_from_dn(group_dn) if group_dn else []
                if ou_path:
                    parent_ou_dn = ','.join([f'OU={ou}' for ou in ou_path[:1]] + group_dn.split(',')[1:])
                    parent_ou_guid = dn_to_guid.get(parent_ou_dn)
                    if parent_ou_guid:
                        edge = self._create_edge_entity(
                            'IN_OU',
                            f'ad_group:{group_guid}',
                            f'ad_ou:{parent_ou_guid}'
                        )
                        output_file.write(json.dumps(edge) + '\n')
                        count += 1
        
        # Export OU hierarchy edges
        if self.config.export_ous:
            ous = self._search_paged(
                self.config.ou_filter,
                ['objectGUID', 'distinguishedName'],
                'ou_edges'
            )
            
            for ou in ous:
                attrs = ou.get('attributes', {})
                ou_guid_raw = attrs.get('objectGUID', [None])[0]
                if not ou_guid_raw:
                    continue
                
                ou_guid = self._guid_to_string(ou_guid_raw)
                ou_dn = attrs.get('distinguishedName', [None])[0]
                
                if not ou_dn:
                    continue
                
                # Find parent OU
                parts = ou_dn.split(',', 1)
                if len(parts) > 1:
                    parent_dn = parts[1]
                    parent_guid = dn_to_guid.get(parent_dn)
                    
                    if parent_guid and parent_dn.upper().startswith('OU='):
                        edge = self._create_edge_entity(
                            'CHILD_OF',
                            f'ad_ou:{ou_guid}',
                            f'ad_ou:{parent_guid}'
                        )
                        output_file.write(json.dumps(edge) + '\n')
                        count += 1
        
        self.exported_count['edges'] = count
        logger.info(f"Exported {count} edges")
        return count
    
    def export_all(self) -> bool:
        """Export all AD objects and relationships"""
        try:
            output_path = Path(self.config.output_file)
            logger.info(f"Exporting to: {output_path}")
            
            with open(output_path, 'w', encoding='utf-8') as f:
                # Export nodes first
                self.export_users(f)
                self.export_groups(f)
                self.export_ous(f)
                
                # Export edges (relationships)
                self.export_edges(f)
            
            logger.info("=" * 60)
            logger.info("Export Summary:")
            logger.info(f"  Users:    {self.exported_count['users']}")
            logger.info(f"  Groups:   {self.exported_count['groups']}")
            logger.info(f"  OUs:      {self.exported_count['ous']}")
            logger.info(f"  Edges:    {self.exported_count['edges']}")
            logger.info(f"  Total:    {sum(self.exported_count.values())}")
            logger.info(f"Output file: {output_path}")
            logger.info("=" * 60)
            
            return True
            
        except Exception as e:
            logger.error(f"Export failed: {e}", exc_info=True)
            return False


def load_config(config_path: str) -> Optional[LDAPExportConfig]:
    """Load configuration from YAML file"""
    if not yaml:
        logger.error("PyYAML not installed. Install with: pip install pyyaml")
        return None
    
    try:
        with open(config_path, 'r') as f:
            config_dict = yaml.safe_load(f)
        return LDAPExportConfig(config_dict)
    except Exception as e:
        logger.error(f"Failed to load config from {config_path}: {e}")
        return None


def main():
    parser = argparse.ArgumentParser(
        description='Export Active Directory/LDAP objects to JSONL for ThemisDB ingestion',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument(
        '--config',
        type=str,
        help='Path to YAML configuration file'
    )
    parser.add_argument(
        '--server',
        type=str,
        help='LDAP server URL (e.g., ldap://dc.example.com)'
    )
    parser.add_argument(
        '--port',
        type=int,
        default=389,
        help='LDAP server port (default: 389)'
    )
    parser.add_argument(
        '--use-ssl',
        action='store_true',
        help='Use SSL/TLS connection'
    )
    parser.add_argument(
        '--bind-dn',
        type=str,
        help='Bind DN for authentication'
    )
    parser.add_argument(
        '--bind-password',
        type=str,
        help='Bind password for authentication'
    )
    parser.add_argument(
        '--base-dn',
        type=str,
        help='Base DN for searches (e.g., DC=example,DC=com)'
    )
    parser.add_argument(
        '--output',
        type=str,
        default='ad_export.jsonl',
        help='Output JSONL file path (default: ad_export.jsonl)'
    )
    parser.add_argument(
        '--max-entries',
        type=int,
        help='Maximum number of entries to export (for testing)'
    )
    parser.add_argument(
        '--generate-file-links',
        action='store_true',
        help='Generate file-link style keys in output'
    )
    
    args = parser.parse_args()
    
    # Load configuration
    if args.config:
        config = load_config(args.config)
        if not config:
            sys.exit(1)
    else:
        # Build config from command line args
        if not args.server or not args.base_dn:
            parser.error("--server and --base-dn are required when not using --config")
        
        config = LDAPExportConfig({
            'server': args.server,
            'port': args.port,
            'use_ssl': args.use_ssl,
            'bind_dn': args.bind_dn or '',
            'bind_password': args.bind_password or '',
            'base_dn': args.base_dn,
            'output_file': args.output,
            'max_entries': args.max_entries,
            'generate_file_links': args.generate_file_links
        })
    
    # Create exporter and run
    exporter = LDAPExporter(config)
    
    try:
        if not exporter.connect():
            logger.error("Failed to connect to LDAP server")
            sys.exit(1)
        
        if not exporter.export_all():
            logger.error("Export failed")
            sys.exit(1)
        
        logger.info("Export completed successfully")
        
    finally:
        exporter.disconnect()


if __name__ == '__main__':
    main()
