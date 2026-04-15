"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     315                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Data models for Blog/Wiki System
Definiert Datenstrukturen für Artikel, Kommentare, Kategorien und Versionen
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import List, Optional
from enum import Enum


class ArticleStatus(Enum):
    """Status eines Artikels"""
    DRAFT = "draft"
    PUBLISHED = "published"
    ARCHIVED = "archived"


@dataclass
class Category:
    """
    Kategorie für Artikel-Organisation
    
    Attributes:
        id: Eindeutige Kategorie-ID
        name: Kategorie-Name
        slug: URL-freundlicher Name
        description: Kategorie-Beschreibung
        article_count: Anzahl der Artikel in dieser Kategorie
    """
    id: str
    name: str
    slug: str
    description: str = ""
    article_count: int = 0
    
    def to_dict(self) -> dict:
        """Konvertiert Category zu Dictionary"""
        return {
            "id": self.id,
            "name": self.name,
            "slug": self.slug,
            "description": self.description,
            "article_count": self.article_count
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'Category':
        """Erstellt Category aus Dictionary"""
        return Category(
            id=data["id"],
            name=data["name"],
            slug=data["slug"],
            description=data.get("description", ""),
            article_count=data.get("article_count", 0)
        )


@dataclass
class Comment:
    """
    Kommentar zu einem Artikel
    
    Attributes:
        id: Eindeutige Kommentar-ID
        article_id: ID des zugehörigen Artikels
        parent_comment_id: ID des Parent-Kommentars (None für Top-Level)
        author: Autor des Kommentars
        content: Kommentar-Inhalt
        created_at: Erstellungszeitpunkt
        replies: Liste von Antwort-Kommentaren
    """
    id: str
    article_id: str
    author: str
    content: str
    created_at: datetime
    parent_comment_id: Optional[str] = None
    replies: List['Comment'] = field(default_factory=list)
    
    def to_dict(self) -> dict:
        """Konvertiert Comment zu Dictionary"""
        return {
            "id": self.id,
            "article_id": self.article_id,
            "parent_comment_id": self.parent_comment_id,
            "author": self.author,
            "content": self.content,
            "created_at": self.created_at.isoformat(),
            "replies": [reply.to_dict() for reply in self.replies]
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'Comment':
        """Erstellt Comment aus Dictionary"""
        created_at = data["created_at"]
        if isinstance(created_at, str):
            created_at = datetime.fromisoformat(created_at)
        
        return Comment(
            id=data["id"],
            article_id=data["article_id"],
            parent_comment_id=data.get("parent_comment_id"),
            author=data["author"],
            content=data["content"],
            created_at=created_at,
            replies=[Comment.from_dict(r) for r in data.get("replies", [])]
        )


@dataclass
class ArticleVersion:
    """
    Version eines Artikels für Historie
    
    Attributes:
        id: Eindeutige Versions-ID
        article_id: ID des zugehörigen Artikels
        version_number: Versions-Nummer
        title: Titel in dieser Version
        content: Inhalt in dieser Version
        changed_by: Autor der Änderung
        changed_at: Zeitpunkt der Änderung
        change_note: Beschreibung der Änderung
    """
    id: str
    article_id: str
    version_number: int
    title: str
    content: str
    changed_by: str
    changed_at: datetime
    change_note: str = ""
    
    def to_dict(self) -> dict:
        """Konvertiert ArticleVersion zu Dictionary"""
        return {
            "id": self.id,
            "article_id": self.article_id,
            "version_number": self.version_number,
            "title": self.title,
            "content": self.content,
            "changed_by": self.changed_by,
            "changed_at": self.changed_at.isoformat(),
            "change_note": self.change_note
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'ArticleVersion':
        """Erstellt ArticleVersion aus Dictionary"""
        changed_at = data["changed_at"]
        if isinstance(changed_at, str):
            changed_at = datetime.fromisoformat(changed_at)
        
        return ArticleVersion(
            id=data["id"],
            article_id=data["article_id"],
            version_number=data["version_number"],
            title=data["title"],
            content=data["content"],
            changed_by=data["changed_by"],
            changed_at=changed_at,
            change_note=data.get("change_note", "")
        )


@dataclass
class Article:
    """
    Blog/Wiki-Artikel
    
    Attributes:
        id: Eindeutige Artikel-ID
        title: Artikel-Titel
        slug: URL-freundlicher Titel
        content: Artikel-Inhalt (Markdown)
        category: Kategorie-Name
        tags: Liste von Tags
        author: Autor des Artikels
        status: Status (draft, published, archived)
        version: Aktuelle Version
        created_at: Erstellungszeitpunkt
        updated_at: Letzter Änderungszeitpunkt
        published_at: Veröffentlichungszeitpunkt
        favorites_count: Anzahl Favoriten
        comments_count: Anzahl Kommentare
    """
    id: str
    title: str
    content: str
    author: str
    slug: str = ""
    category: str = "Uncategorized"
    tags: List[str] = field(default_factory=list)
    status: ArticleStatus = ArticleStatus.DRAFT
    version: int = 1
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None
    published_at: Optional[datetime] = None
    favorites_count: int = 0
    comments_count: int = 0
    
    def __post_init__(self):
        """Initialisiert abgeleitete Felder"""
        if not self.slug:
            self.slug = self._generate_slug(self.title)
        if not self.created_at:
            self.created_at = datetime.now()
        if not self.updated_at:
            self.updated_at = datetime.now()
    
    @staticmethod
    def _generate_slug(title: str) -> str:
        """
        Generiert URL-freundlichen Slug aus Titel
        
        Args:
            title: Artikel-Titel
            
        Returns:
            URL-freundlicher Slug
        """
        slug = title.lower()
        # Ersetze Umlaute
        replacements = {
            'ä': 'ae', 'ö': 'oe', 'ü': 'ue',
            'ß': 'ss', ' ': '-'
        }
        for old, new in replacements.items():
            slug = slug.replace(old, new)
        # Entferne nicht-alphanumerische Zeichen
        slug = ''.join(c for c in slug if c.isalnum() or c == '-')
        # Entferne mehrfache Bindestriche
        while '--' in slug:
            slug = slug.replace('--', '-')
        return slug.strip('-')
    
    def to_dict(self) -> dict:
        """Konvertiert Article zu Dictionary"""
        return {
            "id": self.id,
            "title": self.title,
            "slug": self.slug,
            "content": self.content,
            "category": self.category,
            "tags": self.tags,
            "author": self.author,
            "status": self.status.value,
            "version": self.version,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
            "published_at": self.published_at.isoformat() if self.published_at else None,
            "favorites_count": self.favorites_count,
            "comments_count": self.comments_count
        }
    
    @staticmethod
    def from_dict(data: dict) -> 'Article':
        """Erstellt Article aus Dictionary"""
        # Parse dates
        created_at = data.get("created_at")
        if created_at and isinstance(created_at, str):
            created_at = datetime.fromisoformat(created_at)
        
        updated_at = data.get("updated_at")
        if updated_at and isinstance(updated_at, str):
            updated_at = datetime.fromisoformat(updated_at)
        
        published_at = data.get("published_at")
        if published_at and isinstance(published_at, str):
            published_at = datetime.fromisoformat(published_at)
        
        # Parse status
        status = data.get("status", "draft")
        if isinstance(status, str):
            status = ArticleStatus(status)
        
        return Article(
            id=data["id"],
            title=data["title"],
            slug=data.get("slug", ""),
            content=data["content"],
            category=data.get("category", "Uncategorized"),
            tags=data.get("tags", []),
            author=data["author"],
            status=status,
            version=data.get("version", 1),
            created_at=created_at,
            updated_at=updated_at,
            published_at=published_at,
            favorites_count=data.get("favorites_count", 0),
            comments_count=data.get("comments_count", 0)
        )
