"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     271                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Soziales Netzwerk Datenmodelle
Datenstrukturen für Benutzer und Freundschaften
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import List, Optional
from enum import Enum


@dataclass
class User:
    """
    Repräsentiert einen Benutzer im sozialen Netzwerk.
    
    Attributes:
        id: Eindeutige Benutzer-ID
        name: Name des Benutzers
        bio: Kurze Biographie
        interests: Liste von Interessen
        location: Standort
        joined: Beitrittsdatum
    """
    id: str
    name: str
    bio: str = ""
    interests: List[str] = field(default_factory=list)
    location: str = ""
    joined: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "id": self.id,
            "name": self.name,
            "bio": self.bio,
            "interests": self.interests,
            "location": self.location,
            "joined": self.joined
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'User':
        """Erstellt User aus Dictionary."""
        return cls(
            id=data.get("id", ""),
            name=data.get("name", ""),
            bio=data.get("bio", ""),
            interests=data.get("interests", []),
            location=data.get("location", ""),
            joined=data.get("joined", "")
        )
    
    def __str__(self) -> str:
        """String-Repräsentation."""
        return self.name
    
    def __repr__(self) -> str:
        """Repr-Repräsentation."""
        return f"User(id='{self.id[:8]}...', name='{self.name}')"


@dataclass
class Friendship:
    """
    Repräsentiert eine Freundschaft zwischen zwei Benutzern.
    
    Attributes:
        from_user: ID des ersten Benutzers
        to_user: ID des zweiten Benutzers
        relationship: Art der Beziehung
        since: Datum seit wann befreundet
        strength: Stärke der Beziehung (0-1)
    """
    from_user: str
    to_user: str
    relationship: str = "friend"
    since: str = field(default_factory=lambda: datetime.utcnow().isoformat() + "Z")
    strength: float = 1.0
    
    def to_dict(self) -> dict:
        """Konvertiert zu Dictionary."""
        return {
            "from_user": self.from_user,
            "to_user": self.to_user,
            "relationship": self.relationship,
            "since": self.since,
            "strength": self.strength
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Friendship':
        """Erstellt Friendship aus Dictionary."""
        return cls(
            from_user=data.get("from_user", ""),
            to_user=data.get("to_user", ""),
            relationship=data.get("relationship", "friend"),
            since=data.get("since", ""),
            strength=float(data.get("strength", 1.0))
        )
    
    def __str__(self) -> str:
        """String-Repräsentation."""
        return f"{self.from_user[:8]} <-> {self.to_user[:8]}"


class GraphAlgorithm:
    """Graph-Algorithmen für soziale Netzwerke."""
    
    @staticmethod
    def friends_of_friends(
        user_id: str,
        users: List[User],
        friendships: List[Friendship],
        max_depth: int = 2
    ) -> List[str]:
        """
        Findet Freunde von Freunden (FoF).
        
        Args:
            user_id: Start-Benutzer-ID
            users: Liste aller Benutzer
            friendships: Liste aller Freundschaften
            max_depth: Maximale Tiefe der Suche
            
        Returns:
            Liste von User-IDs (Freunde von Freunden)
        """
        # Baue Adjazenzliste
        adj = {}
        for user in users:
            adj[user.id] = set()
        
        for friendship in friendships:
            adj[friendship.from_user].add(friendship.to_user)
            adj[friendship.to_user].add(friendship.from_user)
        
        # BFS mit Tiefenbegrenzung
        visited = {user_id}
        queue = [(user_id, 0)]
        fof = []
        
        while queue:
            current, depth = queue.pop(0)
            
            if depth >= max_depth:
                continue
            
            for neighbor in adj.get(current, []):
                if neighbor not in visited:
                    visited.add(neighbor)
                    if depth + 1 == max_depth:
                        fof.append(neighbor)
                    queue.append((neighbor, depth + 1))
        
        return fof
    
    @staticmethod
    def shortest_path(
        start_id: str,
        end_id: str,
        users: List[User],
        friendships: List[Friendship]
    ) -> Optional[List[str]]:
        """
        Findet kürzesten Pfad zwischen zwei Benutzern.
        
        Args:
            start_id: Start-Benutzer-ID
            end_id: Ziel-Benutzer-ID
            users: Liste aller Benutzer
            friendships: Liste aller Freundschaften
            
        Returns:
            Liste von User-IDs auf kürzestem Pfad oder None
        """
        # Baue Adjazenzliste
        adj = {}
        for user in users:
            adj[user.id] = set()
        
        for friendship in friendships:
            adj[friendship.from_user].add(friendship.to_user)
            adj[friendship.to_user].add(friendship.from_user)
        
        # BFS für kürzesten Pfad
        visited = {start_id}
        queue = [(start_id, [start_id])]
        
        while queue:
            current, path = queue.pop(0)
            
            if current == end_id:
                return path
            
            for neighbor in adj.get(current, []):
                if neighbor not in visited:
                    visited.add(neighbor)
                    queue.append((neighbor, path + [neighbor]))
        
        return None
    
    @staticmethod
    def friend_recommendations(
        user_id: str,
        users: List[User],
        friendships: List[Friendship],
        top_n: int = 5
    ) -> List[str]:
        """
        Empfiehlt Freunde basierend auf gemeinsamen Freunden.
        
        Args:
            user_id: Benutzer-ID
            users: Liste aller Benutzer
            friendships: Liste aller Freundschaften
            top_n: Anzahl der Empfehlungen
            
        Returns:
            Liste von User-IDs als Empfehlungen
        """
        # Baue Adjazenzliste
        adj = {}
        for user in users:
            adj[user.id] = set()
        
        for friendship in friendships:
            adj[friendship.from_user].add(friendship.to_user)
            adj[friendship.to_user].add(friendship.from_user)
        
        # Direkte Freunde
        direct_friends = adj.get(user_id, set())
        
        # Zähle gemeinsame Freunde
        recommendations = {}
        for friend in direct_friends:
            for fof in adj.get(friend, []):
                if fof != user_id and fof not in direct_friends:
                    recommendations[fof] = recommendations.get(fof, 0) + 1
        
        # Sortiere nach Anzahl gemeinsamer Freunde
        sorted_recs = sorted(
            recommendations.items(),
            key=lambda x: x[1],
            reverse=True
        )
        
        return [user_id for user_id, count in sorted_recs[:top_n]]
