"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     312                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Client für Kontaktmanager
Client-Operationen für Kontakte
"""

import json
import csv
from typing import List, Optional
from pathlib import Path
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from models import Contact, ContactCategory


# Configuration constants
DEFAULT_TIMEOUT = 10  # seconds
DEFAULT_RETRY_COUNT = 3


class ContactClient:
    """
    Client für Kontakt-Operationen in ThemisDB.
    
    Attributes:
        base_url (str): Basis-URL für ThemisDB API
        session (requests.Session): HTTP-Session mit Retry-Logik
        timeout (int): Default timeout
    """
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 8080,
        protocol: str = "http",
        timeout: int = DEFAULT_TIMEOUT
    ):
        """Initialisiert den Contact Client."""
        self.base_url = f"{protocol}://{host}:{port}"
        self.timeout = timeout
        self.session = self._create_session()
    
    def _create_session(self) -> requests.Session:
        """Erstellt HTTP-Session mit Retry-Logik."""
        session = requests.Session()
        retry_strategy = Retry(
            total=DEFAULT_RETRY_COUNT,
            backoff_factor=0.5,
            status_forcelist=[429, 500, 502, 503, 504],
        )
        adapter = HTTPAdapter(max_retries=retry_strategy)
        session.mount("http://", adapter)
        session.mount("https://", adapter)
        return session
    
    def health_check(self) -> bool:
        """Prüft Verbindung zum Server."""
        try:
            response = self.session.get(
                f"{self.base_url}/health",
                timeout=5
            )
            return response.status_code == 200
        except Exception:
            return False
    
    def create_contact(self, contact: Contact) -> Contact:
        """
        Erstellt einen neuen Kontakt.
        
        Args:
            contact: Contact-Objekt
            
        Returns:
            Gespeicherter Kontakt
            
        Raises:
            Exception: Bei Fehler
        """
        entity_key = f"contacts:{contact.id}"
        contact_data = contact.to_dict()
        # Remove id from data
        contact_data_clean = {k: v for k, v in contact_data.items() if k != 'id'}
        
        payload = {
            "blob": json.dumps(contact_data_clean)
        }
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return contact
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create contact: {str(e)}")
    
    def get_contact(self, contact_id: str) -> Optional[Contact]:
        """
        Ruft einen Kontakt ab.
        
        Args:
            contact_id: Kontakt-ID
            
        Returns:
            Contact-Objekt oder None
            
        Raises:
            Exception: Bei Fehler
        """
        entity_key = f"contacts:{contact_id}"
        
        try:
            response = self.session.get(
                f"{self.base_url}/entities/{entity_key}",
                timeout=self.timeout
            )
            
            if response.status_code == 404:
                return None
            
            response.raise_for_status()
            
            data = response.json()
            if "blob" in data:
                contact_data = json.loads(data["blob"])
                contact_data["id"] = contact_id
                return Contact.from_dict(contact_data)
            
            return None
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to get contact: {str(e)}")
    
    def update_contact(self, contact: Contact) -> Contact:
        """
        Aktualisiert einen Kontakt.
        
        Args:
            contact: Contact mit neuen Daten
            
        Returns:
            Aktualisierter Kontakt
            
        Raises:
            Exception: Bei Fehler
        """
        from datetime import datetime
        contact.updated_at = datetime.utcnow().isoformat() + "Z"
        return self.create_contact(contact)
    
    def delete_contact(self, contact_id: str) -> bool:
        """
        Löscht einen Kontakt.
        
        Args:
            contact_id: Kontakt-ID
            
        Returns:
            True bei Erfolg
            
        Raises:
            Exception: Bei Fehler
        """
        entity_key = f"contacts:{contact_id}"
        
        try:
            response = self.session.delete(
                f"{self.base_url}/entities/{entity_key}",
                timeout=self.timeout
            )
            
            if response.status_code in [200, 204, 404]:
                return True
            
            response.raise_for_status()
            return True
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to delete contact: {str(e)}")


class ExportHandler:
    """Handler für Export/Import von Kontakten."""
    
    @staticmethod
    def export_json(contacts: List[Contact], filepath: str) -> None:
        """
        Exportiert Kontakte als JSON.
        
        Args:
            contacts: Liste von Kontakten
            filepath: Ziel-Dateipfad
        """
        data = [contact.to_dict() for contact in contacts]
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
    
    @staticmethod
    def import_json(filepath: str) -> List[Contact]:
        """
        Importiert Kontakte aus JSON.
        
        Args:
            filepath: Quell-Dateipfad
            
        Returns:
            Liste von Kontakten
        """
        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)
        return [Contact.from_dict(item) for item in data]
    
    @staticmethod
    def export_csv(contacts: List[Contact], filepath: str) -> None:
        """
        Exportiert Kontakte als CSV.
        
        Args:
            contacts: Liste von Kontakten
            filepath: Ziel-Dateipfad
        """
        fieldnames = [
            'first_name', 'last_name', 'email', 'phone',
            'street', 'city', 'postal_code', 'country',
            'category', 'is_favorite', 'notes'
        ]
        
        with open(filepath, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            
            for contact in contacts:
                row = {
                    'first_name': contact.first_name,
                    'last_name': contact.last_name,
                    'email': contact.email,
                    'phone': contact.phone,
                    'street': contact.address.street,
                    'city': contact.address.city,
                    'postal_code': contact.address.postal_code,
                    'country': contact.address.country,
                    'category': contact.category.value,
                    'is_favorite': contact.is_favorite,
                    'notes': contact.notes
                }
                writer.writerow(row)
    
    @staticmethod
    def import_csv(filepath: str) -> List[Contact]:
        """
        Importiert Kontakte aus CSV.
        
        Args:
            filepath: Quell-Dateipfad
            
        Returns:
            Liste von Kontakten
        """
        import uuid
        from models import Address
        
        contacts = []
        with open(filepath, 'r', newline='', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            
            for row in reader:
                address = Address(
                    street=row.get('street', ''),
                    city=row.get('city', ''),
                    postal_code=row.get('postal_code', ''),
                    country=row.get('country', '')
                )
                
                contact = Contact(
                    id=str(uuid.uuid4()),
                    first_name=row.get('first_name', ''),
                    last_name=row.get('last_name', ''),
                    email=row.get('email', ''),
                    phone=row.get('phone', ''),
                    address=address,
                    category=ContactCategory(row.get('category', 'other')),
                    is_favorite=row.get('is_favorite', 'False') == 'True',
                    notes=row.get('notes', '')
                )
                contacts.append(contact)
        
        return contacts
