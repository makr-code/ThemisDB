"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     290                                            ║
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
Data models for the Coding Platform.

This module defines the data structures for code snippets, projects,
documentation, and scraping jobs.
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import List, Optional, Dict, Any
from enum import Enum


class SourceType(Enum):
    """Type of code source."""
    GITHUB = "github"
    GITHUB_GIST = "github_gist"
    GITLAB = "gitlab"
    STACKOVERFLOW = "stackoverflow"
    CUSTOM = "custom"
    OFFICIAL_DOCS = "official_docs"
    MANUAL = "manual"


class Visibility(Enum):
    """Snippet visibility level."""
    PUBLIC = "public"
    PRIVATE = "private"
    TEAM = "team"


class JobStatus(Enum):
    """Scraping job status."""
    PENDING = "pending"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"
    CANCELLED = "cancelled"


@dataclass
class CodeSnippet:
    """Represents a code snippet."""
    id: str
    title: str
    code: str
    language: str
    description: str = ""
    framework: Optional[str] = None
    tags: List[str] = field(default_factory=list)
    embedding: Optional[List[float]] = None
    metadata: Dict[str, Any] = field(default_factory=dict)
    stats: Dict[str, int] = field(default_factory=lambda: {
        "views": 0,
        "copies": 0,
        "likes": 0
    })
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None
    visibility: Visibility = Visibility.PUBLIC
    owner_id: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for API/DB."""
        return {
            "id": self.id,
            "title": self.title,
            "description": self.description,
            "code": self.code,
            "language": self.language,
            "framework": self.framework,
            "tags": self.tags,
            "embedding": self.embedding,
            "metadata": self.metadata,
            "stats": self.stats,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
            "visibility": self.visibility.value,
            "owner_id": self.owner_id
        }

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'CodeSnippet':
        """Create from dictionary."""
        created_at = data.get("created_at")
        if created_at and isinstance(created_at, str):
            created_at = datetime.fromisoformat(created_at)
        
        updated_at = data.get("updated_at")
        if updated_at and isinstance(updated_at, str):
            updated_at = datetime.fromisoformat(updated_at)
        
        visibility = data.get("visibility", "public")
        if isinstance(visibility, str):
            visibility = Visibility(visibility)
        
        return cls(
            id=data["id"],
            title=data["title"],
            code=data["code"],
            language=data["language"],
            description=data.get("description", ""),
            framework=data.get("framework"),
            tags=data.get("tags", []),
            embedding=data.get("embedding"),
            metadata=data.get("metadata", {}),
            stats=data.get("stats", {"views": 0, "copies": 0, "likes": 0}),
            created_at=created_at,
            updated_at=updated_at,
            visibility=visibility,
            owner_id=data.get("owner_id")
        )


@dataclass
class ProjectFile:
    """Represents a file in a project."""
    path: str
    content: str
    language: str
    size: int


@dataclass
class Project:
    """Represents a code project."""
    id: str
    name: str
    description: str
    language: str
    framework: Optional[str] = None
    files: List[ProjectFile] = field(default_factory=list)
    structure: Dict[str, Any] = field(default_factory=dict)
    dependencies: List[str] = field(default_factory=list)
    readme: str = ""
    tags: List[str] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for API/DB."""
        return {
            "id": self.id,
            "name": self.name,
            "description": self.description,
            "language": self.language,
            "framework": self.framework,
            "files": [
                {
                    "path": f.path,
                    "content": f.content,
                    "language": f.language,
                    "size": f.size
                }
                for f in self.files
            ],
            "structure": self.structure,
            "dependencies": self.dependencies,
            "readme": self.readme,
            "tags": self.tags,
            "metadata": self.metadata,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None
        }


@dataclass
class Documentation:
    """Represents technical documentation."""
    id: str
    title: str
    content: str
    doc_type: str  # guide, tutorial, reference, api_doc
    language: str
    framework: Optional[str] = None
    sections: List[Dict[str, Any]] = field(default_factory=list)
    embedding: Optional[List[float]] = None
    metadata: Dict[str, Any] = field(default_factory=dict)
    related_snippets: List[str] = field(default_factory=list)
    related_projects: List[str] = field(default_factory=list)
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for API/DB."""
        return {
            "id": self.id,
            "title": self.title,
            "content": self.content,
            "type": self.doc_type,
            "language": self.language,
            "framework": self.framework,
            "sections": self.sections,
            "embedding": self.embedding,
            "metadata": self.metadata,
            "related_snippets": self.related_snippets,
            "related_projects": self.related_projects,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None
        }


@dataclass
class ScrapingJobConfig:
    """Configuration for a scraping job."""
    max_depth: int = 3
    file_patterns: List[str] = field(default_factory=lambda: ["*.py", "*.js", "*.md"])
    exclude_patterns: List[str] = field(default_factory=lambda: ["test_*", "*_test.py"])
    min_file_size: int = 100
    max_file_size: int = 100000
    max_files: Optional[int] = None


@dataclass
class ScrapingJobResults:
    """Results of a scraping job."""
    snippets_created: int = 0
    projects_created: int = 0
    docs_created: int = 0
    duplicates_found: int = 0
    errors: int = 0
    error_messages: List[str] = field(default_factory=list)


@dataclass
class ScrapingJob:
    """Represents a web scraping job."""
    id: str
    job_type: str  # github_repo, stackoverflow, docs_site
    source_url: str
    status: JobStatus = JobStatus.PENDING
    config: ScrapingJobConfig = field(default_factory=ScrapingJobConfig)
    results: ScrapingJobResults = field(default_factory=ScrapingJobResults)
    started_at: Optional[datetime] = None
    completed_at: Optional[datetime] = None
    error_log: List[str] = field(default_factory=list)
    progress: float = 0.0  # 0.0 to 1.0

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for API/DB."""
        return {
            "id": self.id,
            "type": self.job_type,
            "source_url": self.source_url,
            "status": self.status.value,
            "config": {
                "max_depth": self.config.max_depth,
                "file_patterns": self.config.file_patterns,
                "exclude_patterns": self.config.exclude_patterns,
                "min_file_size": self.config.min_file_size,
                "max_file_size": self.config.max_file_size,
                "max_files": self.config.max_files
            },
            "results": {
                "snippets_created": self.results.snippets_created,
                "projects_created": self.results.projects_created,
                "docs_created": self.results.docs_created,
                "duplicates_found": self.results.duplicates_found,
                "errors": self.results.errors,
                "error_messages": self.results.error_messages
            },
            "started_at": self.started_at.isoformat() if self.started_at else None,
            "completed_at": self.completed_at.isoformat() if self.completed_at else None,
            "error_log": self.error_log,
            "progress": self.progress
        }
