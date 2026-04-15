"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     209                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Client für Soziales Netzwerk
Client-Operationen für Benutzer und Freundschaften
"""

import json
from typing import List, Optional
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from models import User, Friendship


# Configuration constants
DEFAULT_TIMEOUT = 10
DEFAULT_RETRY_COUNT = 3


class SocialNetworkClient:
    """
    Client für Social-Network-Operationen in ThemisDB.
    """
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 8080,
        protocol: str = "http",
        timeout: int = DEFAULT_TIMEOUT
    ):
        """Initialisiert den SocialNetworkClient."""
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
    
    # User operations
    def create_user(self, user: User) -> User:
        """Erstellt einen neuen Benutzer."""
        entity_key = f"users:{user.id}"
        user_data = user.to_dict()
        user_data_clean = {k: v for k, v in user_data.items() if k != 'id'}
        
        payload = {"blob": json.dumps(user_data_clean)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return user
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create user: {str(e)}")
    
    def get_user(self, user_id: str) -> Optional[User]:
        """Ruft einen Benutzer ab."""
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
                user_data["id"] = user_id
                return User.from_dict(user_data)
            
            return None
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to get user: {str(e)}")
    
    def update_user(self, user: User) -> User:
        """Aktualisiert einen Benutzer."""
        return self.create_user(user)
    
    def delete_user(self, user_id: str) -> bool:
        """Löscht einen Benutzer."""
        entity_key = f"users:{user_id}"
        
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
            raise Exception(f"Failed to delete user: {str(e)}")
    
    # Friendship operations
    def create_friendship(self, friendship: Friendship) -> Friendship:
        """Erstellt eine Freundschaft."""
        # Bidirektional speichern
        entity_key = f"friendships:{friendship.from_user}:{friendship.to_user}"
        friendship_data = friendship.to_dict()
        
        payload = {"blob": json.dumps(friendship_data)}
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            
            # Auch umgekehrte Richtung speichern
            reverse_key = f"friendships:{friendship.to_user}:{friendship.from_user}"
            reverse_data = {
                "from_user": friendship.to_user,
                "to_user": friendship.from_user,
                "relationship": friendship.relationship,
                "since": friendship.since,
                "strength": friendship.strength
            }
            reverse_payload = {"blob": json.dumps(reverse_data)}
            
            self.session.put(
                f"{self.base_url}/entities/{reverse_key}",
                json=reverse_payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            
            return friendship
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to create friendship: {str(e)}")
    
    def delete_friendship(self, user1_id: str, user2_id: str) -> bool:
        """Löscht eine Freundschaft."""
        try:
            # Beide Richtungen löschen
            entity_key1 = f"friendships:{user1_id}:{user2_id}"
            entity_key2 = f"friendships:{user2_id}:{user1_id}"
            
            self.session.delete(
                f"{self.base_url}/entities/{entity_key1}",
                timeout=self.timeout
            )
            
            self.session.delete(
                f"{self.base_url}/entities/{entity_key2}",
                timeout=self.timeout
            )
            
            return True
        except requests.exceptions.RequestException as e:
            raise Exception(f"Failed to delete friendship: {str(e)}")
