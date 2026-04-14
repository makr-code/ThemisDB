"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            news_researcher.py                                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     535                                            ║
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
News Research Module

Provides functionality to fetch and analyze news articles for moral debate.
Supports multiple sources: RSS feeds, NewsAPI, and sample data.
"""

from typing import List, Optional
from datetime import datetime, timedelta
import requests
from models import NewsArticle

try:
    import feedparser
    HAS_FEEDPARSER = True
except ImportError:
    HAS_FEEDPARSER = False


class NewsResearcher:
    """
    Fetches and filters news articles for moral analysis.
    
    This module can integrate with various news APIs, RSS feeds,
    or use sample data for demonstration purposes.
    """
    
    # Default RSS feeds for German and international news
    DEFAULT_RSS_FEEDS = [
        "https://www.tagesschau.de/xml/rss2/",
        "http://feeds.bbci.co.uk/news/world/rss.xml",
        "https://www.zeit.de/index",
    ]
    
    def __init__(
        self,
        api_key: Optional[str] = None,
        rss_feeds: Optional[List[str]] = None,
        use_rss: bool = True
    ):
        """
        Initialize the news researcher.
        
        Args:
            api_key: Optional API key for news services (e.g., NewsAPI)
            rss_feeds: List of RSS feed URLs to use
            use_rss: Whether to use RSS feeds as primary source
        """
        self.api_key = api_key
        self.session = requests.Session()
        self.use_rss = use_rss and HAS_FEEDPARSER
        self.rss_feeds = rss_feeds or self.DEFAULT_RSS_FEEDS
    
    def fetch_recent_news(
        self,
        category: str = "general",
        limit: int = 10,
        language: str = "de"
    ) -> List[NewsArticle]:
        """
        Fetches recent news articles.
        
        Args:
            category: News category (general, technology, politics, etc.)
            limit: Maximum number of articles
            language: Language code (de, en, etc.)
        
        Returns:
            List of NewsArticle objects
        """
        # Try RSS feeds first if enabled
        if self.use_rss:
            try:
                return self._fetch_from_rss(limit)
            except Exception as e:
                print(f"RSS fetch failed: {e}, falling back to API/samples")
        
        # Try API if key is available
        if self.api_key:
            return self._fetch_from_api(category, limit, language)
        
        # Fall back to sample articles
        return self._get_sample_articles(category, limit)
    
    def search_news(
        self,
        query: str,
        limit: int = 10,
        language: str = "de"
    ) -> List[NewsArticle]:
        """
        Searches for news articles matching a query.
        
        Args:
            query: Search query
            limit: Maximum number of articles
            language: Language code
        
        Returns:
            List of NewsArticle objects
        """
        if self.api_key:
            return self._search_api(query, limit, language)
        else:
            # Filter sample articles by query
            all_articles = self._get_sample_articles("general", 50)
            query_lower = query.lower()
            filtered = [
                a for a in all_articles
                if query_lower in a.title.lower() or query_lower in a.content.lower()
            ]
            return filtered[:limit]
    
    def filter_by_ethical_topics(
        self,
        articles: List[NewsArticle],
        topics: List[str]
    ) -> List[NewsArticle]:
        """
        Filters articles by ethical topics.
        
        Args:
            articles: List of articles to filter
            topics: List of ethical topic keywords
        
        Returns:
            Filtered list of articles
        """
        filtered = []
        topics_lower = [t.lower() for t in topics]
        
        for article in articles:
            article_text = (article.title + " " + article.content).lower()
            if any(topic in article_text for topic in topics_lower):
                article.ethical_topics.extend(
                    [t for t in topics if t.lower() in article_text]
                )
                filtered.append(article)
        
        return filtered
    
    def _fetch_from_api(
        self,
        category: str,
        limit: int,
        language: str
    ) -> List[NewsArticle]:
        """
        Fetches news from an actual API (e.g., NewsAPI).
        
        Args:
            category: News category
            limit: Maximum articles
            language: Language code
        
        Returns:
            List of NewsArticle objects
        """
        # Example using NewsAPI (https://newsapi.org)
        url = "https://newsapi.org/v2/top-headlines"
        params = {
            'apiKey': self.api_key,
            'category': category,
            'language': language,
            'pageSize': limit
        }
        
        try:
            response = self.session.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            articles = []
            for item in data.get('articles', []):
                article = NewsArticle(
                    title=item.get('title', ''),
                    content=item.get('content') or item.get('description', ''),
                    source=item.get('source', {}).get('name', ''),
                    url=item.get('url', ''),
                    category=category,
                    summary=item.get('description', '')
                )
                
                # Parse date
                pub_date = item.get('publishedAt')
                if pub_date:
                    article.published_date = datetime.fromisoformat(
                        pub_date.replace('Z', '+00:00')
                    )
                
                articles.append(article)
            
            return articles
        
        except Exception as e:
            print(f"Error fetching news: {e}")
            return self._get_sample_articles(category, limit)
    
    def _search_api(
        self,
        query: str,
        limit: int,
        language: str
    ) -> List[NewsArticle]:
        """
        Searches news using an API.
        
        Args:
            query: Search query
            limit: Maximum articles
            language: Language code
        
        Returns:
            List of NewsArticle objects
        """
        url = "https://newsapi.org/v2/everything"
        params = {
            'apiKey': self.api_key,
            'q': query,
            'language': language,
            'pageSize': limit,
            'sortBy': 'publishedAt'
        }
        
        try:
            response = self.session.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            articles = []
            for item in data.get('articles', []):
                article = NewsArticle(
                    title=item.get('title', ''),
                    content=item.get('content') or item.get('description', ''),
                    source=item.get('source', {}).get('name', ''),
                    url=item.get('url', ''),
                    summary=item.get('description', '')
                )
                
                pub_date = item.get('publishedAt')
                if pub_date:
                    article.published_date = datetime.fromisoformat(
                        pub_date.replace('Z', '+00:00')
                    )
                
                articles.append(article)
            
            return articles
        
        except Exception as e:
            print(f"Error searching news: {e}")
            return []
    
    def _fetch_from_rss(self, limit: int) -> List[NewsArticle]:
        """
        Fetches news from RSS feeds.
        
        Args:
            limit: Maximum number of articles
        
        Returns:
            List of NewsArticle objects
        """
        if not HAS_FEEDPARSER:
            raise ImportError("feedparser not installed. Install with: pip install feedparser")
        
        all_articles = []
        articles_per_feed = max(1, limit // len(self.rss_feeds))
        
        for feed_url in self.rss_feeds:
            try:
                feed = feedparser.parse(feed_url)
                
                for entry in feed.entries[:articles_per_feed]:
                    article = self._parse_rss_entry(entry, feed_url)
                    if article:
                        all_articles.append(article)
                
            except Exception as e:
                print(f"Error parsing RSS feed {feed_url}: {e}")
                continue
        
        # Sort by date and limit
        all_articles.sort(key=lambda x: x.published_date or datetime.min, reverse=True)
        return all_articles[:limit]
    
    def _parse_rss_entry(self, entry, feed_url: str) -> Optional[NewsArticle]:
        """
        Parses an RSS entry into a NewsArticle.
        
        Args:
            entry: RSS entry from feedparser
            feed_url: URL of the RSS feed
        
        Returns:
            NewsArticle or None if parsing fails
        """
        try:
            article = NewsArticle()
            
            # Title
            article.title = entry.get('title', '')
            
            # Content - try various fields
            content = (
                entry.get('content', [{}])[0].get('value', '') or
                entry.get('summary', '') or
                entry.get('description', '')
            )
            article.content = self._clean_html(content)
            article.summary = article.content[:300] if len(article.content) > 300 else article.content
            
            # URL
            article.url = entry.get('link', '')
            
            # Source - extract from feed URL
            if 'tagesschau' in feed_url:
                article.source = 'Tagesschau'
            elif 'bbc' in feed_url:
                article.source = 'BBC News'
            elif 'zeit' in feed_url:
                article.source = 'Zeit Online'
            elif 'spiegel' in feed_url:
                article.source = 'Spiegel'
            else:
                article.source = entry.get('source', {}).get('title', 'RSS Feed')
            
            # Date
            if hasattr(entry, 'published_parsed') and entry.published_parsed:
                article.published_date = datetime(*entry.published_parsed[:6])
            elif hasattr(entry, 'updated_parsed') and entry.updated_parsed:
                article.published_date = datetime(*entry.updated_parsed[:6])
            else:
                article.published_date = datetime.now()
            
            # Category - try to extract from tags
            if hasattr(entry, 'tags') and entry.tags:
                article.category = entry.tags[0].get('term', 'general')
            else:
                article.category = 'general'
            
            return article
            
        except Exception as e:
            print(f"Error parsing RSS entry: {e}")
            return None
    
    def _clean_html(self, text: str) -> str:
        """
        Removes HTML tags from text.
        
        Args:
            text: Text with potential HTML tags
        
        Returns:
            Clean text
        """
        import re
        # Remove HTML tags
        text = re.sub('<[^<]+?>', '', text)
        # Remove extra whitespace
        text = ' '.join(text.split())
        return text
    
    def _get_sample_articles(self, category: str, limit: int) -> List[NewsArticle]:
        """
        Returns sample news articles for demonstration.
        
        Args:
            category: News category
            limit: Maximum articles
        
        Returns:
            List of sample NewsArticle objects
        """
        sample_articles = [
            NewsArticle(
                title="KI-gesteuerte Entscheidungen im Gesundheitswesen: Chancen und Risiken",
                content="Künstliche Intelligenz revolutioniert die medizinische Diagnostik. "
                       "Algorithmen können Krankheiten früher erkennen als menschliche Ärzte. "
                       "Doch wer trägt die Verantwortung bei Fehldiagnosen? Können Maschinen "
                       "ethische Entscheidungen in Grenzsituationen treffen? Die Debatte über "
                       "den Einsatz von KI in der Medizin wirft grundlegende Fragen über "
                       "Autonomie, Verantwortung und das Wesen medizinischer Fürsorge auf.",
                source="Medizin & Ethik Journal",
                category="technology",
                summary="KI im Gesundheitswesen wirft ethische Fragen auf.",
                ethical_topics=["Autonomy", "Responsibility", "Healthcare"],
                published_date=datetime.now() - timedelta(days=1)
            ),
            NewsArticle(
                title="Klimawandel: Individuelle vs. Kollektive Verantwortung",
                content="Der Klimawandel erfordert dringendes Handeln. Ist es moralisch "
                       "vertretbar, individuelle Freiheiten einzuschränken, um das Gemeinwohl "
                       "zu schützen? Sollten wohlhabende Nationen mehr zahlen? Die Diskussion "
                       "berührt Fragen der Gerechtigkeit zwischen Generationen, globaler "
                       "Verantwortung und der Balance zwischen persönlicher Freiheit und "
                       "kollektivem Nutzen.",
                source="Umwelt & Gesellschaft",
                category="environment",
                summary="Klimakrise wirft Fragen nach individueller und kollektiver Verantwortung auf.",
                ethical_topics=["Justice", "Responsibility", "Environment"],
                published_date=datetime.now() - timedelta(days=2)
            ),
            NewsArticle(
                title="Soziale Medien und Meinungsfreiheit: Wo sind die Grenzen?",
                content="Tech-Konzerne moderieren Inhalte auf ihren Plattformen. Ist dies "
                       "Zensur oder notwendiger Schutz? Die Debatte über Hassrede, Desinformation "
                       "und Meinungsfreiheit ist komplexer denn je. Welche ethischen Prinzipien "
                       "sollten die Content-Moderation leiten? Wie balanciert man freie Meinungsäußerung "
                       "mit dem Schutz vor Schaden?",
                source="Digital Rights Watch",
                category="technology",
                summary="Content-Moderation wirft ethische Fragen über Meinungsfreiheit auf.",
                ethical_topics=["Freedom", "Harm", "Censorship"],
                published_date=datetime.now() - timedelta(days=3)
            ),
            NewsArticle(
                title="Selbstfahrende Autos: Das Trolley-Problem in der Realität",
                content="Autonome Fahrzeuge müssen in Sekundenbruchteilen Entscheidungen treffen, "
                       "die Leben betreffen können. Wie sollten solche Algorithmen programmiert werden? "
                       "Wessen Leben hat Vorrang in einer unvermeidbaren Unfallsituation? Diese "
                       "technologische Entwicklung bringt das klassische Trolley-Problem aus der "
                       "Philosophie in die Realität.",
                source="Auto & Technik",
                category="technology",
                summary="Autonome Fahrzeuge werfen ethische Dilemma-Fragen auf.",
                ethical_topics=["Trolley Problem", "Life and Death", "Technology"],
                published_date=datetime.now() - timedelta(days=4)
            ),
            NewsArticle(
                title="Tierrechte und Massentierhaltung: Ethische Perspektiven",
                content="Die Debatte über Tierrechte intensiviert sich. Ist die industrielle "
                       "Tierhaltung moralisch vertretbar? Haben Tiere intrinsische Rechte oder "
                       "sind sie reine Ressourcen? Die Diskussion berührt Fragen über Leidensfähigkeit, "
                       "moralischen Status und unsere Pflichten gegenüber nicht-menschlichen Lebewesen.",
                source="Ethik & Natur",
                category="society",
                summary="Tierrechte-Debatte wirft Fragen über moralischen Status auf.",
                ethical_topics=["Animal Rights", "Suffering", "Moral Status"],
                published_date=datetime.now() - timedelta(days=5)
            ),
            NewsArticle(
                title="Künstliche Intelligenz und Arbeitsmarkt: Soziale Gerechtigkeit",
                content="Automation bedroht Millionen von Arbeitsplätzen. Haben wir eine "
                       "moralische Verpflichtung, betroffene Arbeitnehmer zu unterstützen? "
                       "Wie sollte der Fortschritt verteilt werden? Die KI-Revolution wirft "
                       "Fragen über Verteilungsgerechtigkeit, soziale Sicherheit und die "
                       "Zukunft der Arbeit auf.",
                source="Wirtschaft & Ethik",
                category="economy",
                summary="KI-Automation wirft Fragen sozialer Gerechtigkeit auf.",
                ethical_topics=["Justice", "Employment", "Technology"],
                published_date=datetime.now() - timedelta(days=6)
            ),
            NewsArticle(
                title="Gentherapie bei Embryonen: Ethische Grenzen der Biotechnologie",
                content="CRISPR ermöglicht die Bearbeitung menschlicher Gene. Sollten wir "
                       "Krankheiten im Erbgut ausschalten? Wo beginnt 'Designer Baby'-Eugenik? "
                       "Die Gentechnik stellt uns vor fundamentale Fragen über menschliche Natur, "
                       "Chancengleichheit und die Grenzen medizinischer Intervention.",
                source="Bioethik Aktuell",
                category="science",
                summary="Gentherapie wirft tiefgreifende ethische Fragen auf.",
                ethical_topics=["Genetic Engineering", "Human Nature", "Enhancement"],
                published_date=datetime.now() - timedelta(days=7)
            ),
            NewsArticle(
                title="Überwachung vs. Privatsphäre: Der Sicherheitsstaat",
                content="Regierungen erweitern Überwachungsbefugnisse zur Terrorbekämpfung. "
                       "Rechtfertigt Sicherheit den Verlust der Privatsphäre? Wie viel Freiheit "
                       "dürfen wir für Sicherheit opfern? Die Balance zwischen öffentlicher "
                       "Sicherheit und individuellen Rechten ist ein zentrales ethisches Dilemma "
                       "unserer Zeit.",
                source="Bürgerrechte & Demokratie",
                category="politics",
                summary="Überwachung wirft Fragen über Privatsphäre und Freiheit auf.",
                ethical_topics=["Privacy", "Security", "Freedom"],
                published_date=datetime.now() - timedelta(days=8)
            ),
            NewsArticle(
                title="Gerechte Verteilung medizinischer Ressourcen in Krisenzeiten",
                content="Pandemien zwingen zu schweren Entscheidungen über die Verteilung "
                       "begrenzter Ressourcen. Nach welchen Kriterien sollten Behandlungen "
                       "priorisiert werden? Alter, Erfolgschancen, sozialer Wert? Diese "
                       "Triage-Situationen offenbaren die Komplexität ethischer Entscheidungen "
                       "unter Ressourcenknappheit.",
                source="Medizinethik Report",
                category="health",
                summary="Ressourcen-Verteilung in Krisen wirft ethische Fragen auf.",
                ethical_topics=["Justice", "Triage", "Healthcare"],
                published_date=datetime.now() - timedelta(days=9)
            ),
            NewsArticle(
                title="Künstliche Intelligenz und algorithmische Vorurteile",
                content="KI-Systeme reproduzieren und verstärken oft gesellschaftliche Vorurteile. "
                       "Sind Entwickler moralisch verantwortlich für diskriminierende Algorithmen? "
                       "Wie schaffen wir faire KI? Die Debatte berührt Fragen über Gerechtigkeit, "
                       "Verantwortung in der Technologieentwicklung und strukturelle Diskriminierung.",
                source="Tech Ethics Institute",
                category="technology",
                summary="Algorithmic Bias wirft Fragen über Gerechtigkeit auf.",
                ethical_topics=["Bias", "Justice", "AI Ethics"],
                published_date=datetime.now() - timedelta(days=10)
            )
        ]
        
        # Filter by category if specified
        if category != "general":
            sample_articles = [a for a in sample_articles if a.category == category]
        
        return sample_articles[:limit]
