"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     235                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Client Wrapper
Einfacher Client für ThemisDB REST API Operationen
"""

import json
from typing import Dict, Any, Optional
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry


# Configuration constants
DEFAULT_TIMEOUT = 10  # seconds
DEFAULT_RETRY_COUNT = 3


class ThemisDBClient:
    """
    Einfacher Client für ThemisDB.
    
    Attributes:
        base_url (str): Basis-URL für ThemisDB API
        session (requests.Session): HTTP-Session mit Retry-Logik
        timeout (int): Default timeout für Requests in Sekunden
    """
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 8080,
        protocol: str = "http",
        timeout: int = DEFAULT_TIMEOUT
    ):
        """
        Initialisiert den ThemisDB Client.
        
        Args:
            host: ThemisDB Server Hostname
            port: ThemisDB Server Port
            protocol: Protokoll (http oder https)
            timeout: Default timeout für Requests in Sekunden
        """
        self.base_url = f"{protocol}://{host}:{port}"
        self.timeout = timeout
        self.session = self._create_session()
    
    def _create_session(self) -> requests.Session:
        """
        Erstellt eine HTTP-Session mit Retry-Logik.
        
        Returns:
            Konfigurierte requests.Session
        """
        session = requests.Session()
        
        # Retry-Strategie: 3 Versuche mit Backoff
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
        """
        Überprüft die Verbindung zum ThemisDB Server.
        
        Returns:
            True wenn Server erreichbar ist, sonst False
        """
        try:
            response = self.session.get(
                f"{self.base_url}/health",
                timeout=5  # Shorter timeout for health checks
            )
            return response.status_code == 200
        except Exception:
            return False
    
    def create_user(self, user_id: str, name: str, email: str) -> Dict[str, Any]:
        """
        Erstellt einen neuen Benutzer in ThemisDB.
        
        Args:
            user_id: Eindeutige Benutzer-ID
            name: Name des Benutzers
            email: Email-Adresse
            
        Returns:
            Dictionary mit Benutzer-Daten
            
        Raises:
            Exception: Bei Fehler in der Kommunikation
        """
        user_data = {
            "name": name,
            "email": email
        }
        
        entity_key = f"users:{user_id}"
        payload = {
            "blob": json.dumps(user_data)
        }
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            
            return {
                "id": user_id,
                **user_data
            }
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create user: {str(e)}")
    
    def get_user(self, user_id: str) -> Optional[Dict[str, Any]]:
        """
        Ruft einen Benutzer aus ThemisDB ab.
        
        Args:
            user_id: Benutzer-ID
            
        Returns:
            Dictionary mit Benutzer-Daten oder None wenn nicht gefunden
            
        Raises:
            Exception: Bei Fehler in der Kommunikation
        """
        entity_key = f"users:{user_id}"
        
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
                user_data = json.loads(data["blob"])
                return {
                    "id": user_id,
                    **user_data
                }
            
            return None
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to get user: {str(e)}")
    
    def update_user(self, user_id: str, name: str, email: str) -> Dict[str, Any]:
        """
        Aktualisiert einen existierenden Benutzer.
        
        Args:
            user_id: Benutzer-ID
            name: Neuer Name
            email: Neue Email
            
        Returns:
            Dictionary mit aktualisierten Benutzer-Daten
            
        Raises:
            Exception: Bei Fehler in der Kommunikation
        """
        # Update verwendet die gleiche PUT-Operation wie Create
        return self.create_user(user_id, name, email)
    
    def delete_user(self, user_id: str) -> bool:
        """
        Löscht einen Benutzer aus ThemisDB.
        
        Args:
            user_id: Benutzer-ID
            
        Returns:
            True wenn erfolgreich gelöscht
            
        Raises:
            Exception: Bei Fehler in der Kommunikation
        """
        entity_key = f"users:{user_id}"
        
        try:
            response = self.session.delete(
                f"{self.base_url}/entities/{entity_key}",
                timeout=self.timeout
            )
            
            # 200 oder 204 = erfolgreich gelöscht
            # 404 = war nicht vorhanden (auch OK)
            if response.status_code in [200, 204, 404]:
                return True
            
            response.raise_for_status()
            return True
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to delete user: {str(e)}")
