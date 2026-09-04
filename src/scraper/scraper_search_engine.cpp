/**
 * @file scraper_search_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scraper/scraper_search_engine.h"
#include <stdexcept>
#include <algorithm>
#include <array>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstring>

#ifdef THEMIS_ENABLE_PUGIXML
#include <pugixml.hpp>
#endif

namespace themis {
namespace scraper {

// ============================================================================
// URL helpers
// ============================================================================

namespace {

/// Encode a single component for use in a query string.
std::string urlEncode(const std::string& s) {
    std::ostringstream out = {};
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out << c;
        } else {
            out << '%'
                << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(c);
        }
    }
    return out.str();
}

/// Extract scheme+host, e.g. "https://example.com" from any URL.
std::string schemeHost(const std::string& url) {
    const std::size_t pos = url.find("://");
    if (pos == std::string::npos) return {};
    const std::size_t slash = url.find('/', pos + 3);
    if (slash == std::string::npos) {
      return url;
    }
    return url.substr(0, slash);
}

/// Resolve href relative to base_url.
std::string resolveUrl(const std::string& href, const std::string& base_url) {
    if (href.empty()) {
      return base_url;
    }
    if (href.compare(0, 4, "http") == 0) {
      return href;
    }
    if (href.compare(0, 2, "//") == 0) {
        const std::size_t colon = base_url.find(':');
        return (colon != std::string::npos ? base_url.substr(0, colon + 1) : "https:") + href;
    }
    if (href.front() == '/') {
      return schemeHost(base_url) + href;
    }
    // Relative path
    const std::size_t slash = base_url.rfind('/');
    if (slash == std::string::npos) {
      return base_url + "/" + href;
    }
    return base_url.substr(0, slash + 1) + href;
}

/// Return lowercase version of a string.
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

/// Build query string from a map of params + a specific key=value pair.
std::string buildQueryString(
        const std::map<std::string, std::string>& fixed,
        const std::string& search_key,
        const std::string& search_val,
        const std::string& page_key,
        int page) {
    std::ostringstream qs = {};
    bool first = true;
    auto append = [&](const std::string& k, const std::string& v) {
        qs << (first ? "" : "&") << urlEncode(k) << "=" << urlEncode(v);
        first = false;
    };
    append(search_key, search_val);
    if (page > 1 && !page_key.empty()) {
      append(page_key, std::to_string(page));
    }
    for (const auto& kv : fixed) {
      append(kv.first, kv.second);
    }
    return qs.str();
}

} // anonymous namespace

// ============================================================================
// HtmlSearchEngine – helpers
// ============================================================================

/*static*/ std::string HtmlSearchEngine::resolveUrl(const std::string& href,
                                                      const std::string& base_url) {
    return ::themis::scraper::resolveUrl(href, base_url);
}

/*static*/ std::string HtmlSearchEngine::urlEncode(const std::string& s) {
    return ::themis::scraper::urlEncode(s);
}

/*static*/ bool HtmlSearchEngine::isSearchInput(const std::string& type,
                                                  const std::string& name,
                                                  const std::string& id,
                                                  const std::string& placeholder) {
    const std::string tl = toLower(type);
    if (tl == "search") {
      return true;
    }
    if (tl != "text" && tl != "") {
      return false;
    }

    // name/id heuristics — English, German, French, Spanish, Italian, Dutch,
    // Polish, Portuguese, Swedish, and common URL/API parameter names.
    const auto looksLikeSearch = [](const std::string& s) {
        const std::string l = toLower(s);
        // Exact matches for very short canonical parameters
        if (l == "q" || l == "s" || l == "search" || l == "query" ||
            l == "suche" || l == "recherche" || l == "buscar" || l == "zoek")
            return true;
        // Prefix match: starts with "q=" or "search"
        if (static_cast<int>(l.size()) >= 1 && l[0] == 'q' && (static_cast<int>(l.size()) == 1 || l[1] == '_' || l[1] == '-'))
            return true;
        // Substring keywords (multilingual)
        static const std::array<std::string, 22> kKeywords = {{
            "search",    // English
            "query",     // English
            "keyword",   // English
            "find",      // English
            "lookup",    // English
            "suche",     // German
            "anfrage",   // German (query)
            "recherche", // French
            "chercher",  // French (to search)
            "busqueda",  // Spanish
            "buscar",    // Spanish (to search)
            "cerca",     // Italian
            "ricerca",   // Italian
            "zoek",      // Dutch
            "szukaj",    // Polish
            "pesquisa",  // Portuguese
            "pesquisar", // Portuguese (to search)
            "sok",       // Swedish/Norwegian
            "søk",       // Norwegian
            "haku",      // Finnish
            "sucht",     // German variant
            "suchfeld",  // German (search field)
        }};
        for (const auto& kw : kKeywords) {
            if (l.find(kw) != std::string::npos) {
              return true;
            }
        }
        return false;
    };

    if (looksLikeSearch(name) || looksLikeSearch(id)) {
      return true;
    }

    // Placeholder heuristics — look for search-intent words in many languages.
    const std::string pl = toLower(placeholder);
    static const std::array<std::string, 18> kPlaceholderKeywords = {{
        "search",     // English
        "find",       // English
        "look",       // English
        "query",      // English
        "such",       // German (partial: "Suche", "suchen", "Suchfeld")
        "eingabe",    // German (input)
        "recherch",   // French (partial: "recherche", "rechercher")
        "cherch",     // French (chercher)
        "busca",      // Spanish/Portuguese
        "busqu",      // Spanish (búsqueda)
        "pesquis",    // Portuguese
        "cerca",      // Italian
        "ricerca",    // Italian
        "zoek",       // Dutch
        "szukaj",     // Polish
        "søk",        // Norwegian
        "haku",       // Finnish
        "ara",        // Turkish
    }};
    for (const auto& kw : kPlaceholderKeywords) {
        if (pl.find(kw) != std::string::npos) {
          return true;
        }
    }
    return false;
}

/*static*/ std::string HtmlSearchEngine::extractText(const std::string& fragment) {
#ifdef THEMIS_ENABLE_PUGIXML
    pugi::xml_document doc;
    doc.load_string(fragment.c_str(),
                    pugi::parse_default | pugi::parse_fragment);
    std::ostringstream out = {};
    std::function<void(const pugi::xml_node&)> walk = [&](const pugi::xml_node& node) {
        for (const auto& child : node.children()) {
            if (child.type() == pugi::node_pcdata ||
                child.type() == pugi::node_cdata) {
                const std::string txt = child.value();
                if (!txt.empty()) { out << txt << ' '; }
            } else {
                walk(child);
            }
        }
    };
    walk(doc);
    return out.str();
#else
    // Minimal fallback: strip tags
    std::string out = {};
    bool in_tag = false;
    for (char c : fragment) {
        if (c == '<')      { in_tag = true; continue; }
        if (c == '>')      { in_tag = false; out += ' '; continue; }
        if (!in_tag) {
          out += c;
        }
    }
    return out;
#endif
}

// ============================================================================
// HtmlSearchEngine – discoverForms
// ============================================================================

std::vector<SearchForm> HtmlSearchEngine::discoverForms(
        const std::string& html,
        const std::string& base_url) const {
    std::vector<SearchForm> result;

#ifdef THEMIS_ENABLE_PUGIXML
    pugi::xml_document doc;
    doc.load_string(html.c_str(), pugi::parse_default | pugi::parse_fragment);

    for (const auto& form_node : doc.select_nodes("//form")) {
        const pugi::xml_node& fn = form_node.node();
        SearchForm sf;
        sf.action_url = resolveUrl(fn.attribute("action").as_string(), base_url);
        if (sf.action_url.empty()) {
          sf.action_url = base_url;
        }
        sf.method  = toLower(fn.attribute("method").as_string("get"));
        sf.enctype = fn.attribute("enctype").as_string(
                         "application/x-www-form-urlencoded");
        // Normalise method to upper-case
        std::transform(sf.method.begin(), sf.method.end(), sf.method.begin(),
                       [](unsigned char c){ return static_cast<char>(std::toupper(c)); });

        // Walk inputs
        for (const auto& inp : fn.select_nodes(".//input")) {
            const pugi::xml_node& in = inp.node();
            const std::string type  = in.attribute("type").as_string("text");
            const std::string name  = in.attribute("name").as_string();
            const std::string id    = in.attribute("id").as_string();
            const std::string ph    = in.attribute("placeholder").as_string();
            const std::string val   = in.attribute("value").as_string();
            const std::string tl    = toLower(type);

            if (tl == "hidden") {
                if (!name.empty()) {
                  sf.hidden_fields[name] = val;
                }
            } else if (isSearchInput(type, name, id, ph) && sf.input_name.empty()) {
                sf.input_name = name;
            }
        }
        // Also check <input type="search"> at top level
        if (sf.input_name.empty()) {
            for (const auto& inp : fn.select_nodes(".//input[@type='search']")) {
                sf.input_name = inp.node().attribute("name").as_string();
                break;
            }
        }
        if (!sf.input_name.empty()) {
            result.push_back(std::move(sf));
        }
    }
#else
    // Minimal fallback: regex-free heuristic scan
    std::size_t pos = 0;
    while ((pos = html.find("<form", pos)) != std::string::npos) {
        const std::size_t form_end = html.find("</form>", pos);
        if (form_end == std::string::npos) {
          break;
        }
        const std::string form_html = html.substr(pos, form_end - pos);

        SearchForm sf;
        // Extract action
        const std::size_t act = form_html.find("action=\"");
        if (act != std::string::npos) {
            const std::size_t vs = act + 8;
            const std::size_t ve = form_html.find('"', vs);
            if (ve != std::string::npos)
                sf.action_url = resolveUrl(form_html.substr(vs, ve - vs), base_url);
        }
        if (sf.action_url.empty()) {
          sf.action_url = base_url;
        }
        sf.method = "GET";
        // Detect search input
        std::size_t ip = 0;
        while ((ip = form_html.find("<input", ip)) != std::string::npos) {
            const std::size_t ie = form_html.find('>', ip);
            const std::string tag = (ie != std::string::npos)
                                  ? form_html.substr(ip, ie - ip) : "";
            const auto attr = [&](const std::string& key) -> std::string {
                const std::string k = key + "=\"";
                std::size_t p = tag.find(k);
                if (p == std::string::npos) return {};
                p += k.size();
                const std::size_t e = tag.find('"', p);
                return (e != std::string::npos) ? tag.substr(p, e - p) : std::string{};
            };
            const std::string type = attr("type");
            const std::string name = attr("name");
            const std::string id   = attr("id");
            const std::string ph   = attr("placeholder");
            if (toLower(type) == "hidden") {
                if (!name.empty()) {
                  sf.hidden_fields[name] = attr("value");
                }
            } else if (isSearchInput(type, name, id, ph) && sf.input_name.empty()) {
                sf.input_name = name;
            }
            ip = (ie != std::string::npos) ? ie : pos + 1;
        }
        if (!sf.input_name.empty()) {
          result.push_back(sf);
        }
        pos = form_end + 1;
    }
#endif
    return result;
}

// ============================================================================
// HtmlSearchEngine – parseResults
// ============================================================================

SearchResultPage HtmlSearchEngine::parseResults(
        const std::string& html,
    const std::string& base_url,
    const std::string& selector) const {
    SearchResultPage page;
    static_cast<void>(base_url);
    static_cast<void>(selector);

#ifdef THEMIS_ENABLE_PUGIXML
    pugi::xml_document doc;
    doc.load_string(html.c_str(), pugi::parse_default | pugi::parse_fragment);

    // Determine XPath from selector hint or try common patterns in order
    const std::vector<std::string> candidates = [&] {
        std::vector<std::string> v = {};

        if (!selector.empty()) {
            // Convert simple CSS class selector to XPath
            if (selector.front() == '.') {
                v.push_back("//*[contains(@class,'" + selector.substr(1) + "')]");
            } else if (selector.front() == '#') {
                v.push_back("//*[@id='" + selector.substr(1) + "']//li");
            } else {
                v.push_back("//" + selector + "//li");
            }
        }
        v.push_back("//ol[contains(@class,'result')]//li");
        v.push_back("//ul[contains(@class,'result')]//li");
        v.push_back("//*[contains(@class,'result-list')]//li");
        v.push_back("//*[contains(@class,'search-result')]");
        v.push_back("//*[@data-result]");
        v.push_back("//article[contains(@class,'result')]");
        v.push_back("//div[contains(@class,'result')]");
        return v;
    }();

    pugi::xpath_node_set items;
    for (const auto& xp : candidates) {
        try {
            items = doc.select_nodes(xp.c_str());
            if (!items.empty()) {
              break;
            }
        } catch (...) {}
    }

    int rank = 1;
    for (const auto& xn : items) {
        const pugi::xml_node& li = xn.node();
        SearchResultItem item;
        item.rank = rank++;

        // Title: first <a> or <h2>/<h3> text
        for (const auto& a : li.select_nodes(".//a")) {
            const std::string href = a.node().attribute("href").as_string();
            const std::string text = extractText(a.node().first_child().value());
            if (!href.empty() && item.url.empty())
                item.url = resolveUrl(href, base_url);
            if (!text.empty() && item.title.empty())
                item.title = text;
        }
        if (item.title.empty()) {
            for (const auto& h : li.select_nodes(".//*[self::h2 or self::h3 or self::h4]")) {
                item.title = extractText(h.node().first_child().value());
                if (!item.title.empty()) {
                  break;
                }
            }
        }

        // Snippet: all remaining text
        item.snippet = extractText(li.first_child().value());
        if (static_cast<int>(item.snippet.size()) > 300) {
          item.snippet = item.snippet.substr(0, 300) + "…";
        }

        // Date: look for <time> or elements with "date"/"datum" class
        for (const auto& t : li.select_nodes(".//*[self::time or contains(@class,'date') or contains(@class,'datum')]")) {
            item.date = toLower(t.node().attribute("datetime").as_string());
            if (item.date.empty()) {
              item.date = extractText(t.node().first_child().value());
            }
            if (!item.date.empty()) {
              break;
            }
        }

        // Source label: look for court/publisher annotation
        for (const auto& t : li.select_nodes(".//*[contains(@class,'court') or contains(@class,'gericht') or contains(@class,'source')]")) {
            item.source_label = extractText(t.node().first_child().value());
            if (!item.source_label.empty()) {
              break;
            }
        }

        if (!item.url.empty()) {
          page.items.push_back(std::move(item));
        }
    }

    // Next-page URL
    const std::vector<std::string> next_xpaths = {
        "//a[@rel='next']",
        "//a[contains(@class,'next')]",
        "//a[contains(@class,'pager-next')]",
        "//li[contains(@class,'next')]//a",
        "//a[contains(text(),'Nächste')]",
        "//a[contains(text(),'Next')]",
        "//a[contains(text(),'weiter')]",
    };
    for (const auto& xp : next_xpaths) {
        try {
            auto nxn = doc.select_nodes(xp.c_str());
            if (!nxn.empty()) {
                const std::string href = nxn.first().node().attribute("href").as_string();
                if (!href.empty()) {
                    page.next_page_url = resolveUrl(href, base_url);
                    page.has_more = true;
                    break;
                }
            }
        } catch (...) {}
    }

    // Total results: look for a result count element
    try {
        auto cnt = doc.select_nodes("//*[contains(@class,'result-count') or contains(@class,'treffer')]");
        if (!cnt.empty()) {
            const std::string txt = extractText(cnt.first().node().first_child().value());
            // Extract first number
            std::string num = {};
            for (char c : txt) {
                if (std::isdigit(c)) {
                  num += c;
                }
                else if (!num.empty()) break;
            }
            if (!num.empty()) {
              page.total_results = std::stoi(num);
            }
        }
    } catch (...) {}

#else
    // Fallback: scan for <a> tags as result items
    std::size_t pos = 0;
    int rank = 1;
    while ((pos = html.find("<a ", pos)) != std::string::npos) {
        const std::size_t ae = html.find('>', pos);
        if (ae == std::string::npos) {
          break;
        }
        const std::string tag = html.substr(pos, ae - pos);
        const std::size_t hi = tag.find("href=\"");
        if (hi != std::string::npos) {
            const std::size_t vs = hi + 6;
            const std::size_t ve = tag.find('"', vs);
            if (ve != std::string::npos) {
                const std::string href = tag.substr(vs, ve - vs);
                if (href.compare(0, 4, "http") == 0) {
                    SearchResultItem item;
                    item.rank = rank++;
                    item.url  = href;
                    const std::size_t te = html.find("</a>", ae);
                    if (te != std::string::npos)
                        item.title = extractText(html.substr(ae + 1, te - ae - 1));
                    page.items.push_back(std::move(item));
                }
            }
        }
        pos = ae + 1;
        if (rank > 50) break; // cap fallback
    }
#endif

    return page;
}

// ============================================================================
// HtmlSearchEngine – URL / body builders
// ============================================================================

std::string HtmlSearchEngine::buildSearchUrl(
        const SearchForm& form,
        const std::string& query,
        int page) const {
    if (form.method == "POST") {
      return form.action_url;
    }
    const std::string qs = buildQueryString(
        form.hidden_fields, form.input_name, query, "p", page);
    const bool has_q = form.action_url.find('?') != std::string::npos;
    return form.action_url + (has_q ? "&" : "?") + qs;
}

std::string HtmlSearchEngine::buildSearchBody(
        const SearchForm& form,
        const std::string& query,
        int page) const {
    return buildQueryString(
        form.hidden_fields, form.input_name, query, "p", page);
}

} // namespace scraper
} // namespace themis


