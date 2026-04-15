"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            knowledge_researcher.py                            ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     646                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Knowledge Researcher - Wissensrecherche für LLM

Recherchiert Hintergrundwissen aus verschiedenen Quellen:
- Wikipedia (philosophische Konzepte, historischer Kontext)
- Wissenschaftliche Publikationen (arXiv, PubMed, Semantic Scholar)
- Stanford Encyclopedia of Philosophy (SEP)
- Internet Encyclopedia of Philosophy (IEP)
- Ethikzentren und philosophische Institute

Stellt dem LLM kontextualisiertes Wissen zur Verfügung.
"""

import requests
from typing import List, Dict, Any, Optional
from dataclasses import dataclass, field
from datetime import datetime
import time
import json
from urllib.parse import quote


@dataclass
class KnowledgeSource:
    """Represents a knowledge source document."""
    
    id: str
    title: str
    content: str
    source_type: str  # wikipedia, arxiv, pubmed, sep, iep, etc.
    url: str
    author: Optional[str] = None
    published_date: Optional[datetime] = None
    relevance_score: float = 0.0
    keywords: List[str] = field(default_factory=list)
    citations: int = 0
    summary: str = ""
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'id': self.id,
            'title': self.title,
            'content': self.content,
            'source_type': self.source_type,
            'url': self.url,
            'author': self.author,
            'published_date': self.published_date.isoformat() if self.published_date else None,
            'relevance_score': self.relevance_score,
            'keywords': self.keywords,
            'citations': self.citations,
            'summary': self.summary
        }


@dataclass
class KnowledgeContext:
    """Aggregated knowledge context for LLM."""
    
    topic: str
    sources: List[KnowledgeSource] = field(default_factory=list)
    key_concepts: Dict[str, str] = field(default_factory=dict)
    historical_context: str = ""
    philosophical_positions: Dict[str, str] = field(default_factory=dict)
    relevant_theories: List[str] = field(default_factory=list)
    created_at: datetime = field(default_factory=datetime.now)
    
    def to_llm_prompt_context(self) -> str:
        """Generate context string for LLM prompt."""
        context = f"**Wissenskontext zum Thema: {self.topic}**\n\n"
        
        if self.key_concepts:
            context += "**Schlüsselkonzepte:**\n"
            for concept, definition in list(self.key_concepts.items())[:5]:
                context += f"- {concept}: {definition}\n"
            context += "\n"
        
        if self.historical_context:
            context += f"**Historischer Kontext:**\n{self.historical_context}\n\n"
        
        if self.philosophical_positions:
            context += "**Philosophische Positionen:**\n"
            for position, description in list(self.philosophical_positions.items())[:3]:
                context += f"- {position}: {description}\n"
            context += "\n"
        
        if self.sources:
            context += f"**Quellen ({len(self.sources)} verfügbar):**\n"
            for source in self.sources[:3]:
                context += f"- [{source.source_type.upper()}] {source.title}\n"
                if source.summary:
                    context += f"  {source.summary[:200]}...\n"
            context += "\n"
        
        return context
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'topic': self.topic,
            'sources': [s.to_dict() for s in self.sources],
            'key_concepts': self.key_concepts,
            'historical_context': self.historical_context,
            'philosophical_positions': self.philosophical_positions,
            'relevant_theories': self.relevant_theories,
            'created_at': self.created_at.isoformat()
        }


class KnowledgeResearcher:
    """
    Recherchiert Hintergrundwissen aus verschiedenen Quellen für LLM-Kontext.
    """
    
    def __init__(
        self,
        wikipedia_lang: str = "de",
        enable_arxiv: bool = True,
        enable_pubmed: bool = False,  # Requires API key
        enable_semantic_scholar: bool = True,
        cache_duration: int = 3600  # Cache for 1 hour
    ):
        """
        Initialize the knowledge researcher.
        
        Args:
            wikipedia_lang: Wikipedia language code (de, en)
            enable_arxiv: Enable arXiv search
            enable_pubmed: Enable PubMed search
            enable_semantic_scholar: Enable Semantic Scholar API
            cache_duration: Cache duration in seconds
        """
        self.wikipedia_lang = wikipedia_lang
        self.enable_arxiv = enable_arxiv
        self.enable_pubmed = enable_pubmed
        self.enable_semantic_scholar = enable_semantic_scholar
        self.cache_duration = cache_duration
        self.cache: Dict[str, KnowledgeContext] = {}
        
        # API endpoints
        self.wikipedia_api = f"https://{wikipedia_lang}.wikipedia.org/w/api.php"
        self.arxiv_api = "http://export.arxiv.org/api/query"
        self.pubmed_api = "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/"
        self.semantic_scholar_api = "https://api.semanticscholar.org/graph/v1"
        self.sep_base = "https://plato.stanford.edu/entries/"
        self.iep_base = "https://iep.utm.edu/"
    
    def research_topic(
        self,
        topic: str,
        keywords: List[str] = None,
        depth: str = "moderate"  # light, moderate, deep
    ) -> KnowledgeContext:
        """
        Research a topic and gather knowledge from multiple sources.
        
        Args:
            topic: Main topic to research
            keywords: Additional keywords for search
            depth: Research depth (light, moderate, deep)
        
        Returns:
            KnowledgeContext with aggregated knowledge
        """
        # Check cache
        cache_key = f"{topic}_{depth}"
        if cache_key in self.cache:
            cached = self.cache[cache_key]
            if (datetime.now() - cached.created_at).seconds < self.cache_duration:
                return cached
        
        context = KnowledgeContext(topic=topic)
        keywords = keywords or []
        
        # Wikipedia as primary source
        wiki_sources = self._search_wikipedia(topic, keywords)
        context.sources.extend(wiki_sources)
        
        # Extract key concepts from Wikipedia
        if wiki_sources:
            context.key_concepts = self._extract_key_concepts(wiki_sources[0])
        
        # Stanford Encyclopedia of Philosophy (web scraping)
        if depth in ["moderate", "deep"]:
            sep_sources = self._search_sep(topic, keywords)
            context.sources.extend(sep_sources)
        
        # Semantic Scholar for academic papers
        if self.enable_semantic_scholar and depth == "deep":
            scholar_sources = self._search_semantic_scholar(topic, keywords)
            context.sources.extend(scholar_sources)
        
        # arXiv for philosophy and ethics papers
        if self.enable_arxiv and depth == "deep":
            arxiv_sources = self._search_arxiv(topic, keywords)
            context.sources.extend(arxiv_sources)
        
        # Extract philosophical positions
        context.philosophical_positions = self._extract_philosophical_positions(
            context.sources
        )
        
        # Generate historical context
        context.historical_context = self._generate_historical_context(
            context.sources
        )
        
        # Sort sources by relevance
        context.sources.sort(key=lambda s: s.relevance_score, reverse=True)
        
        # Cache the result
        self.cache[cache_key] = context
        
        return context
    
    def _search_wikipedia(
        self,
        topic: str,
        keywords: List[str]
    ) -> List[KnowledgeSource]:
        """
        Search Wikipedia for articles.
        
        Args:
            topic: Topic to search
            keywords: Additional keywords
        
        Returns:
            List of KnowledgeSource objects
        """
        sources = []
        
        try:
            # Search for article
            search_query = f"{topic} {' '.join(keywords[:3])}"
            params = {
                'action': 'query',
                'list': 'search',
                'srsearch': search_query,
                'format': 'json',
                'srlimit': 3
            }
            
            response = requests.get(self.wikipedia_api, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            if 'query' in data and 'search' in data['query']:
                for result in data['query']['search']:
                    page_title = result['title']
                    
                    # Get page content
                    content_params = {
                        'action': 'query',
                        'titles': page_title,
                        'prop': 'extracts|info',
                        'exintro': True,
                        'explaintext': True,
                        'inprop': 'url',
                        'format': 'json'
                    }
                    
                    content_response = requests.get(
                        self.wikipedia_api,
                        params=content_params,
                        timeout=10
                    )
                    content_data = content_response.json()
                    
                    pages = content_data.get('query', {}).get('pages', {})
                    for page_id, page_data in pages.items():
                        if page_id != '-1':
                            source = KnowledgeSource(
                                id=f"wiki_{page_id}",
                                title=page_data.get('title', ''),
                                content=page_data.get('extract', '')[:5000],
                                source_type='wikipedia',
                                url=page_data.get('fullurl', ''),
                                relevance_score=0.8,
                                summary=page_data.get('extract', '')[:300]
                            )
                            sources.append(source)
                    
                    time.sleep(0.5)  # Rate limiting
        
        except Exception as e:
            print(f"Wikipedia search error: {e}")
        
        return sources
    
    def _search_sep(
        self,
        topic: str,
        keywords: List[str]
    ) -> List[KnowledgeSource]:
        """
        Search Stanford Encyclopedia of Philosophy.
        
        Note: SEP doesn't have a public API, so this provides URLs to relevant entries.
        In a production system, you would implement web scraping or use their official access.
        
        Args:
            topic: Topic to search
            keywords: Additional keywords
        
        Returns:
            List of KnowledgeSource objects
        """
        sources = []
        
        # Map common topics to SEP entries
        sep_entries = {
            'kant': 'kant',
            'kantian': 'kant-moral',
            'utilitarismus': 'utilitarianism-history',
            'utilitarian': 'consequentialism',
            'deontology': 'ethics-deontological',
            'virtue': 'ethics-virtue',
            'aristotle': 'aristotle-ethics',
            'rawls': 'rawls',
            'contractualism': 'contractualism',
            'moral realism': 'moral-realism',
            'meta-ethics': 'metaethics',
            'ethics': 'ethics-virtue'
        }
        
        topic_lower = topic.lower()
        for key, entry in sep_entries.items():
            if key in topic_lower or any(key in kw.lower() for kw in keywords):
                source = KnowledgeSource(
                    id=f"sep_{entry}",
                    title=f"SEP: {entry.replace('-', ' ').title()}",
                    content=f"Stanford Encyclopedia of Philosophy entry on {entry}. "
                            f"Comprehensive academic resource covering historical development, "
                            f"key arguments, and contemporary debates.",
                    source_type='sep',
                    url=f"{self.sep_base}{entry}/",
                    relevance_score=0.9,
                    summary=f"Authoritative philosophical encyclopedia entry"
                )
                sources.append(source)
        
        return sources
    
    def _search_semantic_scholar(
        self,
        topic: str,
        keywords: List[str]
    ) -> List[KnowledgeSource]:
        """
        Search Semantic Scholar for academic papers.
        
        Args:
            topic: Topic to search
            keywords: Additional keywords
        
        Returns:
            List of KnowledgeSource objects
        """
        sources = []
        
        try:
            # Semantic Scholar API search
            query = f"{topic} ethics philosophy {' '.join(keywords[:2])}"
            url = f"{self.semantic_scholar_api}/paper/search"
            params = {
                'query': query,
                'limit': 5,
                'fields': 'title,abstract,authors,year,citationCount,url'
            }
            
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            if 'data' in data:
                for paper in data['data']:
                    authors = ', '.join([a.get('name', '') for a in paper.get('authors', [])[:3]])
                    
                    source = KnowledgeSource(
                        id=f"scholar_{paper.get('paperId', '')}",
                        title=paper.get('title', ''),
                        content=paper.get('abstract', ''),
                        source_type='semantic_scholar',
                        url=paper.get('url', ''),
                        author=authors,
                        published_date=datetime(paper.get('year', 2020), 1, 1) if paper.get('year') else None,
                        relevance_score=0.7,
                        citations=paper.get('citationCount', 0),
                        summary=paper.get('abstract', '')[:300] if paper.get('abstract') else ''
                    )
                    sources.append(source)
            
            time.sleep(1)  # Rate limiting
        
        except Exception as e:
            print(f"Semantic Scholar search error: {e}")
        
        return sources
    
    def _search_arxiv(
        self,
        topic: str,
        keywords: List[str]
    ) -> List[KnowledgeSource]:
        """
        Search arXiv for philosophy and ethics papers.
        
        Args:
            topic: Topic to search
            keywords: Additional keywords
        
        Returns:
            List of KnowledgeSource objects
        """
        sources = []
        
        try:
            # arXiv search (mainly cs.CY - Computers and Society, which includes ethics)
            query = f"all:{topic} AND (ethics OR philosophy)"
            params = {
                'search_query': query,
                'start': 0,
                'max_results': 5,
                'sortBy': 'relevance'
            }
            
            response = requests.get(self.arxiv_api, params=params, timeout=10)
            response.raise_for_status()
            
            # Parse Atom XML response
            import xml.etree.ElementTree as ET
            root = ET.fromstring(response.content)
            
            # XML namespaces
            ns = {
                'atom': 'http://www.w3.org/2005/Atom',
                'arxiv': 'http://arxiv.org/schemas/atom'
            }
            
            for entry in root.findall('atom:entry', ns):
                title = entry.find('atom:title', ns).text.strip() if entry.find('atom:title', ns) is not None else ''
                summary = entry.find('atom:summary', ns).text.strip() if entry.find('atom:summary', ns) is not None else ''
                url = entry.find('atom:id', ns).text if entry.find('atom:id', ns) is not None else ''
                
                authors = []
                for author in entry.findall('atom:author', ns):
                    name = author.find('atom:name', ns)
                    if name is not None:
                        authors.append(name.text)
                
                published = entry.find('atom:published', ns)
                pub_date = None
                if published is not None:
                    try:
                        pub_date = datetime.fromisoformat(published.text.replace('Z', '+00:00'))
                    except:
                        pass
                
                arxiv_id = url.split('/')[-1] if url else ''
                
                source = KnowledgeSource(
                    id=f"arxiv_{arxiv_id}",
                    title=title,
                    content=summary,
                    source_type='arxiv',
                    url=url,
                    author=', '.join(authors[:3]),
                    published_date=pub_date,
                    relevance_score=0.65,
                    summary=summary[:300]
                )
                sources.append(source)
            
            time.sleep(1)  # Rate limiting
        
        except Exception as e:
            print(f"arXiv search error: {e}")
        
        return sources
    
    def _extract_key_concepts(self, source: KnowledgeSource) -> Dict[str, str]:
        """
        Extract key concepts from a source (simplified version).
        In production, use NLP/LLM for better extraction.
        
        Args:
            source: KnowledgeSource to extract from
        
        Returns:
            Dictionary of concept -> definition
        """
        concepts = {}
        
        # Simple heuristic: look for sentences with "ist" or "bedeutet"
        sentences = source.content.split('.')
        for sentence in sentences[:10]:
            sentence = sentence.strip()
            if ' ist ' in sentence or ' bedeutet ' in sentence:
                # Try to extract concept and definition
                parts = sentence.split(' ist ' if ' ist ' in sentence else ' bedeutet ')
                if len(parts) == 2:
                    concept = parts[0].strip()[-50:]  # Last 50 chars before "ist"
                    definition = parts[1].strip()[:200]  # First 200 chars after
                    if len(concept) > 5 and len(definition) > 10:
                        concepts[concept] = definition
        
        return concepts
    
    def _extract_philosophical_positions(
        self,
        sources: List[KnowledgeSource]
    ) -> Dict[str, str]:
        """
        Extract philosophical positions from sources.
        
        Args:
            sources: List of knowledge sources
        
        Returns:
            Dictionary of position -> description
        """
        positions = {}
        
        # Keywords for different philosophical schools
        school_keywords = {
            'Kantian': ['kategorischer imperativ', 'pflicht', 'autonomie', 'würde'],
            'Utilitarismus': ['nutzen', 'konsequenzen', 'wohlergehen', 'glück'],
            'Tugendethik': ['tugend', 'charakter', 'eudaimonia', 'phronesis'],
            'Kontraktualismus': ['vertrag', 'konsens', 'fairness', 'urzustand'],
            'Care Ethics': ['fürsorge', 'beziehung', 'empathie', 'verantwortung']
        }
        
        for source in sources[:5]:
            content_lower = source.content.lower()
            for school, keywords in school_keywords.items():
                if any(kw in content_lower for kw in keywords):
                    # Extract relevant sentence
                    sentences = source.content.split('.')
                    for sentence in sentences:
                        if any(kw in sentence.lower() for kw in keywords):
                            positions[school] = sentence.strip()[:300]
                            break
        
        return positions
    
    def _generate_historical_context(
        self,
        sources: List[KnowledgeSource]
    ) -> str:
        """
        Generate historical context from sources.
        
        Args:
            sources: List of knowledge sources
        
        Returns:
            Historical context string
        """
        context_parts = []
        
        # Look for historical information in sources
        historical_keywords = [
            'jahrhundert', 'geschichte', 'entwicklung', 'ursprung',
            'tradition', 'historisch', 'entstanden'
        ]
        
        for source in sources[:3]:
            sentences = source.content.split('.')
            for sentence in sentences:
                if any(kw in sentence.lower() for kw in historical_keywords):
                    context_parts.append(sentence.strip())
                    if len(context_parts) >= 3:
                        break
            if len(context_parts) >= 3:
                break
        
        return ' '.join(context_parts[:3]) if context_parts else ""
    
    def get_llm_context_for_topic(
        self,
        topic: str,
        keywords: List[str] = None,
        depth: str = "moderate"
    ) -> str:
        """
        Get formatted context string ready for LLM prompt.
        
        Args:
            topic: Topic to research
            keywords: Additional keywords
            depth: Research depth
        
        Returns:
            Formatted context string for LLM
        """
        context = self.research_topic(topic, keywords, depth)
        return context.to_llm_prompt_context()
    
    def search_specific_philosopher(self, philosopher_name: str) -> KnowledgeContext:
        """
        Research a specific philosopher.
        
        Args:
            philosopher_name: Name of philosopher (e.g., "Immanuel Kant")
        
        Returns:
            KnowledgeContext about the philosopher
        """
        keywords = ["philosophie", "ethik", "werke"]
        return self.research_topic(philosopher_name, keywords, depth="deep")
    
    def search_ethical_concept(self, concept: str) -> KnowledgeContext:
        """
        Research a specific ethical concept.
        
        Args:
            concept: Ethical concept (e.g., "Autonomie", "Gerechtigkeit")
        
        Returns:
            KnowledgeContext about the concept
        """
        keywords = ["ethik", "moral", "philosophie"]
        return self.research_topic(concept, keywords, depth="moderate")
