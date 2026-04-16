"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     431                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Client Wrapper.

This module provides a simplified interface to interact with ThemisDB
for the Coding Platform application.
"""

import requests
from typing import List, Dict, Any, Optional
from datetime import datetime
import uuid

from models import CodeSnippet, Project, Documentation, ScrapingJob


class ThemisDBClient:
    """
    Client for interacting with ThemisDB.
    
    This is a wrapper around the ThemisDB HTTP API that provides
    convenient methods for code snippet management.
    """
    
    def __init__(self, host: str = "localhost", port: int = 8080, api_token: Optional[str] = None):
        """
        Initialize ThemisDB client.
        
        Args:
            host: ThemisDB server hostname
            port: ThemisDB server port
            api_token: Optional API token for authentication
        """
        self.base_url = f"http://{host}:{port}/api/v1"
        self.session = requests.Session()
        if api_token:
            self.session.headers.update({"Authorization": f"Bearer {api_token}"})
    
    def health_check(self) -> bool:
        """Check if ThemisDB server is healthy."""
        try:
            response = self.session.get(f"http://{self.base_url.split('/api')[0]}/health", timeout=5)
            return response.status_code == 200
        except requests.RequestException:
            return False
    
    # Snippet Methods
    
    def create_snippet(self, snippet: CodeSnippet) -> CodeSnippet:
        """
        Create a new code snippet.
        
        Args:
            snippet: CodeSnippet object to create
            
        Returns:
            Created snippet with assigned ID
        """
        if not snippet.id:
            snippet.id = str(uuid.uuid4())
        if not snippet.created_at:
            snippet.created_at = datetime.now()
        snippet.updated_at = datetime.now()
        
        response = self.session.post(
            f"{self.base_url}/snippets",
            json=snippet.to_dict()
        )
        response.raise_for_status()
        return CodeSnippet.from_dict(response.json()["data"])
    
    def get_snippet(self, snippet_id: str) -> Optional[CodeSnippet]:
        """Get a snippet by ID."""
        try:
            response = self.session.get(f"{self.base_url}/snippets/{snippet_id}")
            response.raise_for_status()
            return CodeSnippet.from_dict(response.json()["data"])
        except requests.RequestException:
            return None
    
    def update_snippet(self, snippet: CodeSnippet) -> CodeSnippet:
        """Update an existing snippet."""
        snippet.updated_at = datetime.now()
        response = self.session.put(
            f"{self.base_url}/snippets/{snippet.id}",
            json=snippet.to_dict()
        )
        response.raise_for_status()
        return CodeSnippet.from_dict(response.json()["data"])
    
    def delete_snippet(self, snippet_id: str) -> bool:
        """Delete a snippet by ID."""
        try:
            response = self.session.delete(f"{self.base_url}/snippets/{snippet_id}")
            response.raise_for_status()
            return True
        except requests.RequestException:
            return False
    
    def list_snippets(
        self,
        language: Optional[str] = None,
        framework: Optional[str] = None,
        tags: Optional[List[str]] = None,
        limit: int = 100,
        offset: int = 0
    ) -> List[CodeSnippet]:
        """
        List snippets with optional filters.
        
        Args:
            language: Filter by programming language
            framework: Filter by framework
            tags: Filter by tags
            limit: Maximum number of results
            offset: Offset for pagination
            
        Returns:
            List of matching snippets
        """
        params = {
            "limit": limit,
            "offset": offset
        }
        if language:
            params["language"] = language
        if framework:
            params["framework"] = framework
        if tags:
            params["tags"] = ",".join(tags)
        
        response = self.session.get(f"{self.base_url}/snippets", params=params)
        response.raise_for_status()
        
        data = response.json()["data"]
        return [CodeSnippet.from_dict(item) for item in data]
    
    def search_snippets(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 10
    ) -> List[Dict[str, Any]]:
        """
        Semantic search for snippets.
        
        Args:
            query: Natural language search query
            language: Optional language filter
            limit: Maximum number of results
            
        Returns:
            List of snippets with similarity scores
        """
        response = self.session.post(
            f"{self.base_url}/snippets/search",
            json={
                "query": query,
                "language": language,
                "limit": limit
            }
        )
        response.raise_for_status()
        return response.json()["data"]
    
    def find_similar(
        self,
        code: str,
        language: str,
        limit: int = 5
    ) -> List[Dict[str, Any]]:
        """
        Find similar code snippets.
        
        Args:
            code: Code to find similar snippets for
            language: Programming language
            limit: Maximum number of results
            
        Returns:
            List of similar snippets with similarity scores
        """
        response = self.session.post(
            f"{self.base_url}/snippets/similar",
            json={
                "code": code,
                "language": language,
                "limit": limit
            }
        )
        response.raise_for_status()
        return response.json()["data"]
    
    # Project Methods
    
    def create_project(self, project: Project) -> Project:
        """Create a new project."""
        if not project.id:
            project.id = str(uuid.uuid4())
        if not project.created_at:
            project.created_at = datetime.now()
        project.updated_at = datetime.now()
        
        response = self.session.post(
            f"{self.base_url}/projects",
            json=project.to_dict()
        )
        response.raise_for_status()
        return project
    
    def get_project(self, project_id: str) -> Optional[Project]:
        """Get a project by ID."""
        try:
            response = self.session.get(f"{self.base_url}/projects/{project_id}")
            response.raise_for_status()
            return Project(**response.json()["data"])
        except requests.RequestException:
            return None
    
    def list_projects(
        self,
        language: Optional[str] = None,
        limit: int = 50
    ) -> List[Project]:
        """List projects with optional filters."""
        params = {"limit": limit}
        if language:
            params["language"] = language
        
        response = self.session.get(f"{self.base_url}/projects", params=params)
        response.raise_for_status()
        
        data = response.json()["data"]
        return [Project(**item) for item in data]
    
    # Documentation Methods
    
    def create_documentation(self, doc: Documentation) -> Documentation:
        """Create new documentation."""
        if not doc.id:
            doc.id = str(uuid.uuid4())
        if not doc.created_at:
            doc.created_at = datetime.now()
        doc.updated_at = datetime.now()
        
        response = self.session.post(
            f"{self.base_url}/documentation",
            json=doc.to_dict()
        )
        response.raise_for_status()
        return doc
    
    def get_documentation(self, doc_id: str) -> Optional[Documentation]:
        """Get documentation by ID."""
        try:
            response = self.session.get(f"{self.base_url}/documentation/{doc_id}")
            response.raise_for_status()
            return Documentation(**response.json()["data"])
        except requests.RequestException:
            return None
    
    # Scraping Job Methods
    
    def create_scraping_job(self, job: ScrapingJob) -> ScrapingJob:
        """Create a new scraping job."""
        if not job.id:
            job.id = str(uuid.uuid4())
        
        response = self.session.post(
            f"{self.base_url}/scraping/jobs",
            json=job.to_dict()
        )
        response.raise_for_status()
        return job
    
    def get_scraping_job(self, job_id: str) -> Optional[ScrapingJob]:
        """Get scraping job status."""
        try:
            response = self.session.get(f"{self.base_url}/scraping/jobs/{job_id}")
            response.raise_for_status()
            data = response.json()["data"]
            # Convert back to ScrapingJob (simplified)
            return data
        except requests.RequestException:
            return None
    
    def list_scraping_jobs(
        self,
        status: Optional[str] = None,
        limit: int = 50
    ) -> List[Dict[str, Any]]:
        """List scraping jobs."""
        params = {"limit": limit}
        if status:
            params["status"] = status
        
        response = self.session.get(f"{self.base_url}/scraping/jobs", params=params)
        response.raise_for_status()
        return response.json()["data"]
    
    def cancel_scraping_job(self, job_id: str) -> bool:
        """Cancel a running scraping job."""
        try:
            response = self.session.delete(f"{self.base_url}/scraping/jobs/{job_id}")
            response.raise_for_status()
            return True
        except requests.RequestException:
            return False
    
    # Statistics Methods
    
    def get_statistics(self) -> Dict[str, Any]:
        """Get platform statistics."""
        try:
            response = self.session.get(f"{self.base_url}/statistics")
            response.raise_for_status()
            return response.json()["data"]
        except requests.RequestException:
            return {
                "total_snippets": 0,
                "total_projects": 0,
                "total_docs": 0,
                "languages": {}
            }


# Mock implementation for development/testing without actual ThemisDB server
class MockThemisDBClient(ThemisDBClient):
    """Mock client for testing without ThemisDB server."""
    
    def __init__(self, *args, **kwargs):
        # Don't call parent __init__ to avoid creating session
        self.snippets = {}
        self.projects = {}
        self.docs = {}
        self.jobs = {}
    
    def health_check(self) -> bool:
        return True
    
    def create_snippet(self, snippet: CodeSnippet) -> CodeSnippet:
        if not snippet.id:
            snippet.id = str(uuid.uuid4())
        if not snippet.created_at:
            snippet.created_at = datetime.now()
        snippet.updated_at = datetime.now()
        self.snippets[snippet.id] = snippet
        return snippet
    
    def get_snippet(self, snippet_id: str) -> Optional[CodeSnippet]:
        return self.snippets.get(snippet_id)
    
    def update_snippet(self, snippet: CodeSnippet) -> CodeSnippet:
        snippet.updated_at = datetime.now()
        self.snippets[snippet.id] = snippet
        return snippet
    
    def delete_snippet(self, snippet_id: str) -> bool:
        if snippet_id in self.snippets:
            del self.snippets[snippet_id]
            return True
        return False
    
    def list_snippets(self, language=None, framework=None, tags=None, limit=100, offset=0):
        snippets = list(self.snippets.values())
        
        if language:
            snippets = [s for s in snippets if s.language == language]
        if framework:
            snippets = [s for s in snippets if s.framework == framework]
        if tags:
            snippets = [s for s in snippets if any(t in s.tags for t in tags)]
        
        return snippets[offset:offset + limit]
    
    def search_snippets(self, query, language=None, limit=10):
        # Simple keyword search for mock
        snippets = list(self.snippets.values())
        if language:
            snippets = [s for s in snippets if s.language == language]
        
        query_lower = query.lower()
        results = []
        for snippet in snippets:
            if (query_lower in snippet.title.lower() or
                query_lower in snippet.description.lower() or
                query_lower in snippet.code.lower()):
                results.append({
                    "snippet": snippet,
                    "score": 0.85  # Mock similarity score
                })
        
        return results[:limit]
    
    def find_similar(self, code, language, limit=5):
        # Mock implementation
        snippets = [s for s in self.snippets.values() if s.language == language]
        return [{"snippet": s, "score": 0.75} for s in snippets[:limit]]
    
    def get_statistics(self):
        languages = {}
        for snippet in self.snippets.values():
            languages[snippet.language] = languages.get(snippet.language, 0) + 1
        
        return {
            "total_snippets": len(self.snippets),
            "total_projects": len(self.projects),
            "total_docs": len(self.docs),
            "languages": languages
        }
