"""
ThemisDB Client for Moral Philosophy Debates

Provides integration with ThemisDB for storing and querying debates,
arguments, and news articles.
"""

import json
from typing import List, Optional, Dict, Any
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
from models import NewsArticle, PhilosophicalArgument, DebateSession


class MoralDebateClient:
    """
    Client for storing and retrieving moral philosophy debates in ThemisDB.
    
    This client provides methods to:
    - Store news articles
    - Save debate sessions and arguments
    - Query past debates
    - Retrieve arguments by philosophy school
    """
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 8080,
        protocol: str = "http",
        timeout: int = 10
    ):
        """
        Initialize the ThemisDB client.
        
        Args:
            host: ThemisDB server hostname
            port: ThemisDB server port
            protocol: Protocol (http or https)
            timeout: Request timeout in seconds
        """
        self.base_url = f"{protocol}://{host}:{port}"
        self.timeout = timeout
        self.session = self._create_session()
    
    def _create_session(self) -> requests.Session:
        """
        Creates an HTTP session with retry logic.
        
        Returns:
            Configured requests.Session
        """
        session = requests.Session()
        
        retry_strategy = Retry(
            total=3,
            backoff_factor=0.5,
            status_forcelist=[429, 500, 502, 503, 504],
        )
        
        adapter = HTTPAdapter(max_retries=retry_strategy)
        session.mount("http://", adapter)
        session.mount("https://", adapter)
        
        return session
    
    def health_check(self) -> bool:
        """
        Checks connection to ThemisDB server.
        
        Returns:
            True if server is reachable
        """
        try:
            response = self.session.get(
                f"{self.base_url}/health",
                timeout=5
            )
            return response.status_code == 200
        except Exception:
            return False
    
    # === News Article Operations ===
    
    def store_news_article(self, article: NewsArticle) -> bool:
        """
        Stores a news article in ThemisDB.
        
        Args:
            article: NewsArticle to store
        
        Returns:
            True if successful
        """
        entity_key = f"news_articles:{article.id}"
        payload = {
            "blob": json.dumps(article.to_dict())
        }
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return True
        except Exception as e:
            print(f"Error storing article: {e}")
            return False
    
    def get_news_article(self, article_id: str) -> Optional[NewsArticle]:
        """
        Retrieves a news article from ThemisDB.
        
        Args:
            article_id: Article ID
        
        Returns:
            NewsArticle or None if not found
        """
        entity_key = f"news_articles:{article_id}"
        
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
                article_data = json.loads(data["blob"])
                return NewsArticle.from_dict(article_data)
            
            return None
        except Exception as e:
            print(f"Error retrieving article: {e}")
            return None
    
    # === Debate Session Operations ===
    
    def store_debate_session(self, session: DebateSession) -> bool:
        """
        Stores a debate session in ThemisDB.
        
        Args:
            session: DebateSession to store
        
        Returns:
            True if successful
        """
        entity_key = f"debate_sessions:{session.id}"
        payload = {
            "blob": json.dumps(session.to_dict())
        }
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return True
        except Exception as e:
            print(f"Error storing debate session: {e}")
            return False
    
    def get_debate_session(self, session_id: str) -> Optional[DebateSession]:
        """
        Retrieves a debate session from ThemisDB.
        
        Args:
            session_id: Session ID
        
        Returns:
            DebateSession or None if not found
        """
        entity_key = f"debate_sessions:{session_id}"
        
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
                session_data = json.loads(data["blob"])
                return DebateSession.from_dict(session_data)
            
            return None
        except Exception as e:
            print(f"Error retrieving debate session: {e}")
            return None
    
    def list_debate_sessions(self, limit: int = 10) -> List[DebateSession]:
        """
        Lists recent debate sessions.
        
        Args:
            limit: Maximum number of sessions to return
        
        Returns:
            List of DebateSession objects
        """
        # In a real implementation, this would use a query or scan operation
        # For now, we return an empty list as we don't have scan capability
        # in this simple client
        return []
    
    # === Argument Operations ===
    
    def store_argument(self, argument: PhilosophicalArgument) -> bool:
        """
        Stores a philosophical argument in ThemisDB.
        
        Args:
            argument: PhilosophicalArgument to store
        
        Returns:
            True if successful
        """
        entity_key = f"arguments:{argument.id}"
        payload = {
            "blob": json.dumps(argument.to_dict())
        }
        
        try:
            response = self.session.put(
                f"{self.base_url}/entities/{entity_key}",
                json=payload,
                headers={"Content-Type": "application/json"},
                timeout=self.timeout
            )
            response.raise_for_status()
            return True
        except Exception as e:
            print(f"Error storing argument: {e}")
            return False
    
    def get_argument(self, argument_id: str) -> Optional[PhilosophicalArgument]:
        """
        Retrieves a philosophical argument from ThemisDB.
        
        Args:
            argument_id: Argument ID
        
        Returns:
            PhilosophicalArgument or None if not found
        """
        entity_key = f"arguments:{argument_id}"
        
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
                arg_data = json.loads(data["blob"])
                return PhilosophicalArgument.from_dict(arg_data)
            
            return None
        except Exception as e:
            print(f"Error retrieving argument: {e}")
            return None
    
    # === Statistics ===
    
    def get_debate_statistics(self) -> Dict[str, Any]:
        """
        Gets statistics about stored debates.
        
        Returns:
            Dictionary with statistics
        """
        # Placeholder - in real implementation would query the database
        return {
            'total_debates': 0,
            'total_arguments': 0,
            'total_articles': 0,
            'consensus_rate': 0.0
        }
