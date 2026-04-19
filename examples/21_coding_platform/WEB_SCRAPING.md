> **Hinweis:** API-Signaturen gegen aktuelle Endpunkte/Typen prüfen. Abweichungen mit `<!-- TODO: verify API -->` markieren.

# Web Scraping & Ingestion Guide

Dieser Guide erklärt detailliert, wie Sie Code-Beispiele aus verschiedenen Internet-Quellen scrapen, parsen und in ThemisDB importieren.

## 📋 Inhaltsverzeichnis

1. [Übersicht](#übersicht)
2. [GitHub Integration](#github-integration)
3. [Stack Overflow Scraping](#stack-overflow-scraping)
4. [Dokumentations-Crawler](#dokumentations-crawler)
5. [Custom Scrapers](#custom-scrapers)
6. [Best Practices](#best-practices)
7. [Rate Limiting & Ethics](#rate-limiting--ethics)

## 1. Übersicht

### Unterstützte Quellen

| Quelle | Typ | API | Auth erforderlich | Rate Limit |
|--------|-----|-----|-------------------|------------|
| GitHub | Repository | ✅ REST API v3 | Optional (empfohlen) | 60/h (5000/h mit Token) |
| GitHub Gist | Code Snippets | ✅ REST API v3 | Optional | 60/h (5000/h mit Token) |
| GitLab | Repository | ✅ REST API v4 | Optional | 10/min |
| Stack Overflow | Q&A | ✅ REST API 2.3 | Optional | 300/day (10000/day mit Key) |
| ReadTheDocs | Documentation | ❌ HTML Scraping | Nein | Keine offizielle |
| DevDocs | Documentation | ✅ JSON API | Nein | Keine offizielle |
| MDN Web Docs | Documentation | ❌ HTML Scraping | Nein | Keine offizielle |
| npm Registry | Packages | ✅ REST API | Nein | Unbegrenzt |
| PyPI | Packages | ✅ JSON API | Nein | Unbegrenzt |

### Architektur

```
┌─────────────────┐
│  Web Scraper    │
│  - URLs Queue   │
│  - Rate Limiter │
│  - Retry Logic  │
└────────┬────────┘
         │
         ├──→ GitHub API
         ├──→ SO API  
         └──→ HTML Parser
              │
    ┌─────────▼─────────┐
    │  Content Parser   │
    │  - Code Detection │
    │  - Language Det.  │
    │  - Metadata Ext.  │
    └─────────┬─────────┘
              │
    ┌─────────▼─────────┐
    │  Code Indexer     │
    │  - Embeddings     │
    │  - Deduplication  │
    │  - Quality Filter │
    └─────────┬─────────┘
              │
    ┌─────────▼─────────┐
    │  ThemisDB         │
    │  - Store Snippets │
    │  - Build Index    │
    └───────────────────┘
```

## 2. GitHub Integration

### Setup

**Personal Access Token erstellen**:
1. Gehen Sie zu https://github.com/settings/tokens
2. Klicken Sie "Generate new token (classic)"
3. Scopes auswählen:
   - `public_repo` - Zugriff auf öffentliche Repositories
   - `read:user` - Benutzerinformationen lesen
4. Token kopieren und in `.env` speichern:
   ```bash
   GITHUB_TOKEN=ghp_xxxxxxxxxxxxxxxxxxxx
   ```

### Repository scrapen

**Einfaches Beispiel**:
```python
from scrapers.github_scraper import GitHubScraper

scraper = GitHubScraper(token=os.getenv('GITHUB_TOKEN'))

# Vollständiges Repository scrapen
job = scraper.scrape_repository(
    url="https://github.com/fastapi/fastapi",
    max_files=100,
    file_patterns=["*.py"],
    exclude_patterns=["test_*", "*_test.py", "tests/*"]
)

print(f"Job ID: {job.id}")
print(f"Status: {job.status}")
```

**Erweiterte Optionen**:
```python
job = scraper.scrape_repository(
    url="https://github.com/django/django",
    branch="main",  # Spezifischer Branch
    max_files=500,
    file_patterns=["*.py", "*.md"],
    exclude_patterns=[
        "test_*.py",
        "tests/*",
        "docs/*",
        ".github/*"
    ],
    min_file_size=100,      # Bytes
    max_file_size=100000,   # Bytes (100KB)
    min_stars=1000,         # Nur populäre Repos
    language="python",
    recursive=True,
    max_depth=10
)
```

**Batch-Scraping (mehrere Repos)**:
```python
repos = [
    "https://github.com/fastapi/fastapi",
    "https://github.com/django/django",
    "https://github.com/flask/flask",
    "https://github.com/pallets/werkzeug"
]

jobs = scraper.scrape_repositories_batch(
    urls=repos,
    max_files_per_repo=100,
    concurrent=3  # 3 Repos gleichzeitig
)

# Status aller Jobs überwachen
for job in jobs:
    print(f"{job.url}: {job.status} - {job.results['snippets_created']} snippets")
```

### GitHub-Suche mit Filters

**Top-Repositories nach Sprache**:
```python
jobs = scraper.search_and_scrape(
    query="language:python stars:>1000",
    max_repos=20,
    sort="stars",
    order="desc"
)
```

**Specific Topics**:
```python
jobs = scraper.search_and_scrape(
    query="topic:fastapi topic:authentication",
    max_repos=10
)
```

**Recent Projects**:
```python
jobs = scraper.search_and_scrape(
    query="language:python created:>2024-01-01 stars:>100",
    max_repos=50
)
```

### Gists scrapen

```python
from scrapers.github_scraper import GitHubGistScraper

gist_scraper = GitHubGistScraper(token=os.getenv('GITHUB_TOKEN'))

# Einzelnes Gist
snippet = gist_scraper.scrape_gist(
    url="https://gist.github.com/user/abc123def456"
)

# User's Gists
snippets = gist_scraper.scrape_user_gists(
    username="torvalds",
    max_gists=50
)

# Public Gists (recent)
snippets = gist_scraper.scrape_public_gists(
    language="python",
    max_gists=100,
    since="2024-01-01"
)
```

### Rate Limiting handhaben

**Automatisches Rate Limiting**:
```python
scraper = GitHubScraper(
    token=os.getenv('GITHUB_TOKEN'),
    rate_limit_strategy="adaptive",  # adaptive, conservative, aggressive
    retry_on_limit=True,
    max_retries=3
)

# Scraper prüft automatisch Rate Limit vor jedem Request
job = scraper.scrape_repository(...)
```

**Manuell Rate Limit prüfen**:
```python
rate_limit = scraper.get_rate_limit()
print(f"Remaining: {rate_limit['remaining']}/{rate_limit['limit']}")
print(f"Reset at: {rate_limit['reset_time']}")

if rate_limit['remaining'] < 100:
    print("Warning: Low rate limit remaining")
    time.sleep(rate_limit['seconds_until_reset'])
```

## 3. Stack Overflow Scraping

### Setup

**API Key beantragen** (optional, erhöht Rate Limit):
1. Gehen Sie zu https://stackapps.com/apps/oauth/register
2. Registrieren Sie Ihre App
3. Kopieren Sie den Key
4. Speichern Sie in `.env`:
   ```bash
   STACKOVERFLOW_KEY=your_key_here
   ```

### Fragen scrapen

**Nach Tags filtern**:
```python
from scrapers.stackoverflow_scraper import StackOverflowScraper

so_scraper = StackOverflowScraper(key=os.getenv('STACKOVERFLOW_KEY'))

job = so_scraper.scrape_questions(
    tags=["python", "asyncio"],
    min_score=10,          # Nur Fragen mit >= 10 Upvotes
    max_questions=50,
    has_accepted_answer=True,
    sort="votes",          # votes, activity, creation
    order="desc"
)
```

**Zeitbereich filtern**:
```python
from datetime import datetime, timedelta

# Fragen der letzten 30 Tage
thirty_days_ago = datetime.now() - timedelta(days=30)

job = so_scraper.scrape_questions(
    tags=["fastapi"],
    from_date=thirty_days_ago,
    min_score=5,
    max_questions=100
)
```

### Antworten extrahieren

```python
# Code-Blöcke aus Antworten extrahieren
snippets = so_scraper.extract_code_from_answers(
    question_id=12345678,
    min_answer_score=5,     # Nur Antworten mit >= 5 Upvotes
    accepted_only=False,
    include_comments=True
)

for snippet in snippets:
    print(f"Language: {snippet.language}")
    print(f"Score: {snippet.metadata['score']}")
    print(snippet.code[:100])
```

### User-Profile scrapen

```python
# Top-Contributor's Code scrapen
snippets = so_scraper.scrape_user_answers(
    user_id=22656,  # Jon Skeet
    min_score=10,
    max_answers=100,
    tags=["python"]  # Optional: nur bestimmte Tags
)
```

### Search-Queries

**Advanced Search**:
```python
job = so_scraper.search_questions(
    query="fastapi authentication jwt",
    tags=["python", "fastapi"],
    min_score=10,
    max_results=50
)
```

**Boolean Operators**:
```python
# Fragen über asyncio ODER async/await
job = so_scraper.search_questions(
    query="asyncio OR async/await",
    tags=["python"]
)

# Fragen über Django UND authentication
job = so_scraper.search_questions(
    query="django AND authentication",
    tags=["django"]
)
```

## 4. Dokumentations-Crawler

### ReadTheDocs crawlen

```python
from scrapers.docs_crawler import ReadTheDocsCrawler

rtd_crawler = ReadTheDocsCrawler()

# Single Documentation
job = rtd_crawler.crawl_documentation(
    base_url="https://fastapi.tiangolo.com/",
    max_depth=3,
    max_pages=100,
    extract_code_examples=True
)
```

**Konfiguration**:
```python
job = rtd_crawler.crawl_documentation(
    base_url="https://docs.python.org/3/library/asyncio.html",
    max_depth=2,
    max_pages=50,
    follow_external=False,  # Nur interne Links
    extract_code_examples=True,
    code_languages=["python"],  # Nur Python Code
    ignore_patterns=[
        "*genindex*",
        "*search*",
        "*modules*"
    ]
)
```

### DevDocs crawlen

```python
from scrapers.docs_crawler import DevDocsCrawler

devdocs = DevDocsCrawler()

# Komplette Dokumentation
docs = devdocs.scrape_documentation(
    slug="python~3.12",  # Python 3.12 docs
    max_entries=500
)

# Mehrere Dokumentationen
docs = devdocs.scrape_multiple(
    slugs=[
        "python~3.12",
        "django~4.2",
        "flask~2.3",
        "fastapi"
    ]
)
```

### MDN Web Docs

```python
from scrapers.docs_crawler import MDNCrawler

mdn = MDNCrawler()

# JavaScript Docs
job = mdn.crawl_documentation(
    base_url="https://developer.mozilla.org/en-US/docs/Web/JavaScript",
    max_depth=3,
    languages=["en"],
    extract_interactive_examples=True
)
```

### Package Documentation

**PyPI Readme & Docs**:
```python
from scrapers.package_scraper import PyPIScraper

pypi = PyPIScraper()

# Package Info + Readme
doc = pypi.scrape_package(
    package_name="fastapi",
    include_readme=True,
    include_changelog=True
)

# Top Packages
docs = pypi.scrape_top_packages(
    category="Web Frameworks",
    max_packages=20
)
```

**npm Registry**:
```python
from scrapers.package_scraper import NPMScraper

npm = NPMScraper()

# Package + Docs
doc = npm.scrape_package(
    package_name="express",
    include_readme=True
)
```

## 5. Custom Scrapers

### Basis-Klasse verwenden

```python
# scrapers/my_custom_scraper.py
from web_scraper import BaseScraper
import requests
from bs4 import BeautifulSoup

class MyCustomScraper(BaseScraper):
    def __init__(self, api_key=None):
        super().__init__()
        self.api_key = api_key
        self.base_url = "https://example.com/api"
    
    def scrape(self, url: str) -> List[CodeSnippet]:
        """
        Implementieren Sie Ihre Scraping-Logik
        """
        snippets = []
        
        # 1. HTML fetchen
        response = self.fetch_with_retry(url)
        soup = BeautifulSoup(response.text, 'html.parser')
        
        # 2. Code-Blöcke finden
        code_blocks = soup.find_all('pre', class_='code')
        
        # 3. Code extrahieren und parsen
        for block in code_blocks:
            code = block.get_text()
            language = self.detect_language(block)
            
            # 4. Snippet erstellen
            snippet = self.create_snippet(
                title=self.generate_title(code),
                code=code,
                language=language,
                source_url=url,
                metadata={
                    'source_type': 'custom',
                    'scraped_at': datetime.now().isoformat()
                }
            )
            
            snippets.append(snippet)
        
        return snippets
    
    def detect_language(self, element) -> str:
        """
        Sprache aus HTML-Element erkennen
        """
        # Aus class-Attribut
        classes = element.get('class', [])
        for cls in classes:
            if cls.startswith('language-'):
                return cls.replace('language-', '')
        
        # Aus data-Attribut
        lang = element.get('data-language')
        if lang:
            return lang
        
        # Fallback: Lexer-basierte Erkennung
        code = element.get_text()
        return self.detect_language_from_code(code)
```

### Verwendung

```python
from scrapers.my_custom_scraper import MyCustomScraper

scraper = MyCustomScraper(api_key="your_key")
snippets = scraper.scrape("https://example.com/tutorials/python")

# In ThemisDB speichern
for snippet in snippets:
    client.create_snippet(snippet)
```

### Fortgeschrittene Features

**Paralleles Scraping**:
```python
from concurrent.futures import ThreadPoolExecutor

class ParallelScraper(BaseScraper):
    def scrape_urls_parallel(self, urls: List[str], max_workers=5):
        with ThreadPoolExecutor(max_workers=max_workers) as executor:
            results = executor.map(self.scrape, urls)
        
        return [snippet for result in results for snippet in result]
```

**Caching**:
```python
from functools import lru_cache
import hashlib

class CachedScraper(BaseScraper):
    @lru_cache(maxsize=1000)
    def scrape_with_cache(self, url: str):
        return self.scrape(url)
    
    def get_cache_key(self, url: str) -> str:
        return hashlib.md5(url.encode()).hexdigest()
```

**JavaScript-rendered Seiten**:
```python
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait

class SeleniumScraper(BaseScraper):
    def __init__(self):
        super().__init__()
        options = webdriver.ChromeOptions()
        options.add_argument('--headless')
        self.driver = webdriver.Chrome(options=options)
    
    def scrape_js_page(self, url: str):
        self.driver.get(url)
        
        # Warten auf spezifisches Element
        wait = WebDriverWait(self.driver, 10)
        code_blocks = wait.until(
            lambda d: d.find_elements(By.CSS_SELECTOR, 'pre.code')
        )
        
        snippets = []
        for block in code_blocks:
            code = block.text
            snippets.append(self.create_snippet(
                code=code,
                source_url=url
            ))
        
        return snippets
    
    def __del__(self):
        self.driver.quit()
```

## 6. Best Practices

### Duplicate Detection

**Content-basiert**:
```python
from code_indexer import CodeIndexer

indexer = CodeIndexer()

def is_duplicate(new_snippet, threshold=0.95):
    """
    Prüft ob Snippet bereits existiert
    """
    similar = indexer.find_similar(
        code=new_snippet.code,
        limit=1
    )
    
    if similar and similar[0]['similarity'] > threshold:
        return True
    
    return False
```

**Hash-basiert**:
```python
import hashlib

def get_code_hash(code: str) -> str:
    """
    Erstellt Hash von normalisiertem Code
    """
    # Whitespace normalisieren
    normalized = ' '.join(code.split())
    return hashlib.sha256(normalized.encode()).hexdigest()

# Verwendung
seen_hashes = set()

for snippet in scraped_snippets:
    code_hash = get_code_hash(snippet.code)
    
    if code_hash in seen_hashes:
        print(f"Duplicate: {snippet.title}")
        continue
    
    seen_hashes.add(code_hash)
    client.create_snippet(snippet)
```

### Quality Filtering

```python
def is_high_quality(snippet) -> bool:
    """
    Filtert niedrig-qualitative Snippets
    """
    code = snippet.code
    
    # Zu kurz
    if len(code) < 50:
        return False
    
    # Zu lang (wahrscheinlich komplette Datei)
    if len(code) > 10000:
        return False
    
    # Nur Kommentare
    lines = code.split('\n')
    code_lines = [l for l in lines if l.strip() and not l.strip().startswith('#')]
    if len(code_lines) < 3:
        return False
    
    # Mindestens eine Funktion/Klasse Definition
    if 'def ' not in code and 'class ' not in code and 'function ' not in code:
        return False
    
    return True
```

### Error Handling

```python
class RobustScraper(BaseScraper):
    def scrape_with_retry(self, url: str, max_retries=3):
        for attempt in range(max_retries):
            try:
                return self.scrape(url)
            except requests.RequestException as e:
                if attempt == max_retries - 1:
                    self.log_error(url, str(e))
                    raise
                
                # Exponential backoff
                time.sleep(2 ** attempt)
            except Exception as e:
                self.log_error(url, str(e))
                raise
```

## 7. Rate Limiting & Ethics

### Rate Limiting Implementation

```python
import time
from collections import deque

class RateLimiter:
    def __init__(self, max_requests, time_window):
        self.max_requests = max_requests
        self.time_window = time_window  # seconds
        self.requests = deque()
    
    def wait_if_needed(self):
        now = time.time()
        
        # Entferne alte Requests
        while self.requests and self.requests[0] < now - self.time_window:
            self.requests.popleft()
        
        # Warten wenn Limit erreicht
        if len(self.requests) >= self.max_requests:
            sleep_time = self.requests[0] + self.time_window - now
            if sleep_time > 0:
                time.sleep(sleep_time)
            self.requests.popleft()
        
        self.requests.append(now)

# Verwendung
limiter = RateLimiter(max_requests=5, time_window=60)  # 5 req/min

for url in urls:
    limiter.wait_if_needed()
    scrape(url)
```

### Ethical Guidelines

**✅ DO:**
- Respektieren Sie `robots.txt`
- Implementieren Sie Rate Limiting
- Fügen Sie User-Agent Header hinzu
- Cachen Sie Ergebnisse
- Scrapen Sie nur öffentliche Daten
- Beachten Sie Lizenzen

**❌ DON'T:**
- Ignorieren Sie nicht Rate Limits
- Überladen Sie Server nicht
- Scrapen Sie keine privaten Daten
- Umgehen Sie keine Authentifizierung
- Entfernen Sie keine Copyright-Hinweise

**robots.txt prüfen**:
```python
from urllib.robotparser import RobotFileParser

def can_fetch(url: str) -> bool:
    rp = RobotFileParser()
    rp.set_url(f"{url}/robots.txt")
    rp.read()
    return rp.can_fetch("*", url)

# Verwendung
if can_fetch("https://example.com/page"):
    scrape("https://example.com/page")
```

**User-Agent setzen**:
```python
headers = {
    'User-Agent': 'ThemisDB Code Scraper/1.0 (contact@example.com)'
}

response = requests.get(url, headers=headers)
```

### Monitoring

**Scraping-Statistiken**:
```python
class ScrapingMonitor:
    def __init__(self):
        self.stats = {
            'requests': 0,
            'successes': 0,
            'failures': 0,
            'snippets_created': 0,
            'duplicates': 0,
            'rate_limit_hits': 0
        }
    
    def log_request(self, url, success, snippets=0, duplicate=False):
        self.stats['requests'] += 1
        if success:
            self.stats['successes'] += 1
            self.stats['snippets_created'] += snippets
            if duplicate:
                self.stats['duplicates'] += 1
        else:
            self.stats['failures'] += 1
    
    def report(self):
        print(f"Total Requests: {self.stats['requests']}")
        print(f"Success Rate: {self.stats['successes']/self.stats['requests']*100:.1f}%")
        print(f"Snippets Created: {self.stats['snippets_created']}")
        print(f"Duplicates: {self.stats['duplicates']}")
```

---

## 📚 Weitere Ressourcen

- [GitHub API Docs](https://docs.github.com/en/rest)
- [Stack Overflow API](https://api.stackexchange.com/docs)
- [Web Scraping Best Practices](https://www.scrapehero.com/web-scraping-best-practices/)
- [robots.txt Specification](https://www.robotstxt.org/)

**Fragen?** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
