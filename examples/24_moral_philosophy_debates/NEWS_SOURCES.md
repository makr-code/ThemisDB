> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# Verlässliche Nachrichtenquellen für Moral Philosophy Debates

Dieses Dokument listet verlässliche Nachrichtenquellen auf, die für das Moral Philosophy Debates System genutzt werden können.

## 🌐 Internationale Nachrichtenagenturen

### 1. NewsAPI.org
- **URL**: https://newsapi.org
- **API**: REST API mit kostenlosem Tier (100 requests/Tag)
- **Abdeckung**: 80+ Länder, 150.000+ Quellen
- **Kategorien**: Business, Entertainment, General, Health, Science, Sports, Technology
- **Sprachen**: Deutsch, Englisch, und 14+ weitere
- **Kosten**: Kostenlos für Entwicklung (bis 100 req/Tag), ab $449/Monat für Produktion
- **Implementation**: Bereits in `news_researcher.py` vorbereitet

**Beispiel-Integration**:
```python
api_key = "YOUR_NEWSAPI_KEY"
researcher = NewsResearcher(api_key=api_key)
articles = researcher.fetch_recent_news(category="technology", language="de")
```

### 2. The Guardian API
- **URL**: https://open-platform.theguardian.com
- **API**: REST API, kostenlos mit Registrierung
- **Abdeckung**: Hochwertige Artikel von The Guardian
- **Rate Limit**: 5.000 requests/Tag (kostenlos)
- **Qualität**: Hervorragender Journalismus, detaillierte Artikel
- **Ethik-Fokus**: Starke Berichterstattung zu ethischen und gesellschaftlichen Themen

**Vorteile für unser System**:
- Kostenlos und zuverlässig
- Gute Abdeckung ethisch relevanter Themen
- Strukturierte API mit guter Dokumentation

### 3. Bing News Search API (Microsoft)
- **URL**: https://www.microsoft.com/en-us/bing/apis/bing-news-search-api
- **API**: REST API über Azure Cognitive Services
- **Abdeckung**: Global, Echtzeit-Nachrichten
- **Kosten**: Kostenloser Tier verfügbar (1.000 Transaktionen/Monat)
- **Qualität**: Aggregiert aus vielen vertrauenswürdigen Quellen

## 🇩🇪 Deutschsprachige Nachrichtenquellen

### 4. Tagesschau API
- **URL**: https://tagesschau.de
- **API**: Inoffizielle APIs verfügbar (z.B. via GitHub)
- **Abdeckung**: Deutsche Nachrichten, ARD
- **Qualität**: Öffentlich-rechtlich, sehr verlässlich
- **Lizenz**: Zu prüfen für kommerzielle Nutzung

**Repositories**:
- https://github.com/AndreasFischer1985/tagesschau-api

### 5. DPA (Deutsche Presse-Agentur)
- **URL**: https://www.dpa.com
- **API**: Kommerziell, professionelle Nutzung
- **Abdeckung**: Deutsche und internationale Nachrichten
- **Qualität**: Sehr hoch, Faktenchecking
- **Kosten**: Kostenpflichtig, Kontakt erforderlich

### 6. Heise Online RSS
- **URL**: https://www.heise.de
- **API**: RSS Feeds (kostenlos)
- **Fokus**: Technologie, IT, digitale Gesellschaft
- **Qualität**: Hervorragend für Tech-Ethik-Themen

## 🆓 Kostenlose und Open-Source-Optionen

### 7. RSS Feeds (Empfohlen für Start)
Viele seriöse Nachrichtenseiten bieten RSS-Feeds:

**Deutsche Quellen**:
- Tagesschau: https://www.tagesschau.de/xml/rss2/
- Zeit Online: https://www.zeit.de/index
- Süddeutsche: https://www.sueddeutsche.de/news/
- Spiegel: https://www.spiegel.de/schlagzeilen/index.rss

**Internationale Quellen**:
- BBC: http://feeds.bbci.co.uk/news/rss.xml
- Reuters: http://feeds.reuters.com/reuters/topNews
- Al Jazeera: https://www.aljazeera.com/xml/rss/all.xml

**Implementierung**:
```python
import feedparser

def fetch_rss_news(feed_url):
    feed = feedparser.parse(feed_url)
    articles = []
    for entry in feed.entries:
        article = NewsArticle(
            title=entry.title,
            content=entry.summary,
            url=entry.link,
            published_date=entry.published_parsed
        )
        articles.append(article)
    return articles
```

### 8. Common Crawl News
- **URL**: https://commoncrawl.org/
- **API**: S3 Buckets mit Crawl-Daten
- **Abdeckung**: Massive Sammlung von Webinhalten
- **Kosten**: Kostenlos (S3-Kosten für Download)
- **Komplexität**: Erfordert eigene Filterung und Verarbeitung

### 9. GDELT Project
- **URL**: https://www.gdeltproject.org/
- **API**: BigQuery, REST API
- **Abdeckung**: Global Event Database, Echtzeit
- **Fokus**: Gesellschaftliche Ereignisse, Konflikte, Trends
- **Kosten**: Kostenlos
- **Besonderheit**: Perfekt für ethisch/politisch relevante Ereignisse

## 📊 Spezialisierte Ethik-Nachrichtenquellen

### 10. Ethik-spezifische Feeds
- **Bioethics News**: https://bioethics.net/
- **Ethics Unwrapped (UT Austin)**: https://ethicsunwrapped.utexas.edu/
- **Markkula Center for Applied Ethics**: https://www.scu.edu/ethics/

### 11. Wissenschaftliche Nachrichtenquellen
- **Science Daily**: https://www.sciencedaily.com/ (RSS verfügbar)
- **Phys.org**: https://phys.org/rss-feed/ (Ethik in Wissenschaft)
- **Nature News**: https://www.nature.com/news (teilweise frei)

## 🔧 Empfohlene Implementierungsstrategie

### Phase 1: Start mit RSS Feeds (Einfach, kostenlos)
```python
# Erweitere news_researcher.py
import feedparser

class RSSNewsResearcher(NewsResearcher):
    def __init__(self, rss_feeds: List[str]):
        super().__init__()
        self.rss_feeds = rss_feeds
    
    def fetch_from_rss(self, limit: int = 10) -> List[NewsArticle]:
        all_articles = []
        for feed_url in self.rss_feeds:
            feed = feedparser.parse(feed_url)
            for entry in feed.entries[:limit]:
                article = self._parse_rss_entry(entry)
                all_articles.append(article)
        return all_articles[:limit]
```

### Phase 2: Integration von NewsAPI (Skalierbar)
- Bereits in `news_researcher.py` vorbereitet
- Kostenloser Development-Zugang
- Einfach zu erweitern

### Phase 3: Guardian API hinzufügen (Qualität)
- Sehr gute Artikelqualität
- Kostenlos mit hohem Limit
- Ethik-relevante Inhalte

## 📋 Bewertungskriterien für Nachrichtenquellen

| Quelle | Kosten | Rate Limit | Qualität | Ethik-Fokus | Empfehlung |
|--------|--------|------------|----------|-------------|------------|
| NewsAPI | Kostenlos* | 100/Tag | Gut | Mittel | ⭐⭐⭐⭐ |
| Guardian API | Kostenlos | 5.000/Tag | Sehr gut | Hoch | ⭐⭐⭐⭐⭐ |
| Bing News | Kostenlos* | 1.000/Monat | Gut | Mittel | ⭐⭐⭐ |
| RSS Feeds | Kostenlos | Unbegrenzt | Variiert | Variiert | ⭐⭐⭐⭐ |
| GDELT | Kostenlos | Hoch | Gut | Sehr hoch | ⭐⭐⭐⭐ |
| Tagesschau RSS | Kostenlos | Unbegrenzt | Sehr gut | Hoch | ⭐⭐⭐⭐⭐ |

*Kostenlos für Entwicklung/geringe Volumina

## 🚀 Quick Start Empfehlung

**Für den Einstieg** (keine API-Keys erforderlich):
```python
# RSS Feeds verwenden
RSS_FEEDS = [
    "https://www.tagesschau.de/xml/rss2/",
    "http://feeds.bbci.co.uk/news/rss.xml",
    "https://www.zeit.de/index"
]

researcher = RSSNewsResearcher(rss_feeds=RSS_FEEDS)
articles = researcher.fetch_from_rss(limit=20)
```

**Für Produktion** (mit API-Key):
```python
# NewsAPI verwenden
api_key = "YOUR_NEWSAPI_KEY"
researcher = NewsResearcher(api_key=api_key)
articles = researcher.fetch_recent_news(
    category="technology",
    language="de",
    limit=20
)
```

## 🔒 Rechtliche Hinweise

- **Robots.txt** beachten beim Scraping
- **API Terms of Service** einhalten
- **Urheberrecht** respektieren (keine vollständigen Artikel speichern)
- **Attribution** bei Nutzung von Inhalten
- Für **kommerzielle Nutzung** Lizenzen prüfen

## 🌟 Beste Wahl für dieses Projekt

**Empfehlung: Kombination aus:**

1. **RSS Feeds** für den Start (Tagesschau, BBC, Zeit)
   - Kostenlos, zuverlässig
   - Keine Rate Limits
   - Gute deutsche Abdeckung

2. **Guardian API** für hochwertige internationale Artikel
   - Kostenlos mit großzügigem Limit
   - Hervorragende Qualität
   - Starker Ethik-Fokus

3. **NewsAPI** als Fallback/Ergänzung
   - Breite Abdeckung
   - Einfache Integration
   - Bereits implementiert

## 📚 Weitere Ressourcen

- **MediaStack API**: https://mediastack.com/ (Alternative zu NewsAPI)
- **Currents API**: https://currentsapi.services/en (Fokus auf aktuelle Nachrichten)
- **GNews API**: https://gnews.io/ (Kostenloser Tier verfügbar)
- **Awesome Public Datasets**: https://github.com/awesomedata/awesome-public-datasets#news

---

**Nächste Schritte**: 
1. RSS Feed Integration implementieren (einfachster Start)
2. Guardian API Key holen (kostenlos)
3. NewsAPI Key für zusätzliche Abdeckung
