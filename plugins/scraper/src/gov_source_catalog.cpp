/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gov_source_catalog.cpp                             ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 18:48:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     468                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c2cc8e90ab  2026-04-02  feat(plugins/scraper): add agentic scraper plugin with go... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "gov_source_catalog.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef THEMIS_ENABLE_YAML
#include <yaml-cpp/yaml.h>
#endif

namespace themis {
namespace scraper {

// ============================================================================
// Constructor
// ============================================================================

GovSourceCatalog::GovSourceCatalog() {
    populateBuiltinBund();
    populateBuiltinBundeslaender();
    populateBuiltinEU();
}

// ============================================================================
// Query helpers
// ============================================================================

const std::vector<GovDataSource> &GovSourceCatalog::all() const {
    return sources_;
}

const GovDataSource *GovSourceCatalog::findById(const std::string &id) const {
    for (const auto &s : sources_) {
        if (s.id == id) {
            return &s;
        }
    }
    return nullptr;
}

std::vector<const GovDataSource *> GovSourceCatalog::byType(GovSourceType type) const {
    std::vector<const GovDataSource *> result = {};

    for (const auto &s : sources_) {
        if (s.type == type) {
            result.push_back(&s);
        }
    }
    return result;
}

std::vector<const GovDataSource *> GovSourceCatalog::byBundesland(const std::string &iso) const {
    std::vector<const GovDataSource *> result = {};

    for (const auto &s : sources_) {
        if (s.bundesland == iso) {
            result.push_back(&s);
        }
    }
    return result;
}

std::vector<const GovDataSource *> GovSourceCatalog::enabled() const {
    std::vector<const GovDataSource *> result = {};

    for (const auto &s : sources_) {
        if (s.enabled) {
            result.push_back(&s);
        }
    }
    return result;
}

std::vector<const GovDataSource *> GovSourceCatalog::byIds(const std::vector<std::string> &ids) const {
    std::vector<const GovDataSource *> result = {};

    for (const auto &id : ids) {
        if (const auto *s = findById(id)) {
            result.push_back(s);
        }
    }
    return result;
}

// ============================================================================
// Mutation
// ============================================================================

void GovSourceCatalog::upsert(GovDataSource source) {
    for (auto &s : sources_) {
        if (s.id == source.id) {
            s = std::move(source);
            return;
        }
    }
    sources_.push_back(std::move(source));
}

bool GovSourceCatalog::setEnabled(const std::string &id, bool en) {
    for (auto &s : sources_) {
        if (s.id == id) {
            s.enabled = en;
            return true;
        }
    }
    return false;
}

// ============================================================================
// YAML loading
// ============================================================================

void GovSourceCatalog::loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("GovSourceCatalog: cannot open '" + path + "'");
    }
    std::ostringstream ss = {};
    ss << f.rdbuf();
    loadFromYaml(ss.str());
}

void GovSourceCatalog::loadFromYaml(const std::string &yaml_content) {
#ifdef THEMIS_ENABLE_YAML
    try {
        YAML::Node root = YAML::Load(yaml_content);
        if (!root["sources"] || !root["sources"].IsSequence())
            return;
        for (const auto &n : root["sources"]) {
            GovDataSource src = {};
            if (n["id"])
                src.id = n["id"].as<std::string>();
            if (n["name"])
                src.name = n["name"].as<std::string>();
            if (n["base_url"])
                src.base_url = n["base_url"].as<std::string>();
            if (n["search_url"])
                src.search_url = n["search_url"].as<std::string>();
            if (n["sitemap_url"])
                src.sitemap_url = n["sitemap_url"].as<std::string>();
            if (n["search_param"])
                src.search_param = n["search_param"].as<std::string>();
            if (n["page_param"])
                src.page_param = n["page_param"].as<std::string>();
            if (n["results_per_page"])
                src.results_per_page = n["results_per_page"].as<int>();
            if (n["form_method"])
                src.form_method = n["form_method"].as<std::string>();
            if (n["result_list_selector"])
                src.result_list_selector = n["result_list_selector"].as<std::string>();
            if (n["next_page_selector"])
                src.next_page_selector = n["next_page_selector"].as<std::string>();
            if (n["api_endpoint"])
                src.api_endpoint = n["api_endpoint"].as<std::string>();
            if (n["api_results_field"])
                src.api_results_field = n["api_results_field"].as<std::string>();
            if (n["api_cursor_field"])
                src.api_cursor_field = n["api_cursor_field"].as<std::string>();
            if (n["api_key_env"])
                src.api_key_env = n["api_key_env"].as<std::string>();
            if (n["notes"])
                src.notes = n["notes"].as<std::string>();
            if (n["requires_auth"])
                src.requires_auth = n["requires_auth"].as<bool>();
            if (n["enabled"])
                src.enabled = n["enabled"].as<bool>();
            if (n["language"])
                src.language = n["language"].as<std::string>();
            if (n["bundesland"])
                src.bundesland = n["bundesland"].as<std::string>();

            if (n["type"]) {
                const std::string t = n["type"].as<std::string>();
                if (t == "BUND")
                    src.type = GovSourceType::BUND;
                else if (t == "BUNDESLAND")
                    src.type = GovSourceType::BUNDESLAND;
                else if (t == "EU")
                    src.type = GovSourceType::EU;
                else
                    src.type = GovSourceType::OTHER;
            }
            if (n["search_style"]) {
                const std::string ss = n["search_style"].as<std::string>();
                if (ss == "REST_JSON")
                    src.search_style = GovSearchStyle::REST_JSON;
                else if (ss == "EURLEX_API")
                    src.search_style = GovSearchStyle::EURLEX_API;
                else if (ss == "LISTING")
                    src.search_style = GovSearchStyle::LISTING;
                else if (ss == "SITEMAP")
                    src.search_style = GovSearchStyle::SITEMAP;
                else
                    src.search_style = GovSearchStyle::HTML_FORM;
            }
            if (n["extra_params"] && n["extra_params"].IsMap()) {
                for (const auto &kv : n["extra_params"])
                    src.extra_params[kv.first.as<std::string>()] = kv.second.as<std::string>();
            }
            if (!src.id.empty())
                upsert(std::move(src));
        }
    } catch (const YAML::Exception &e) {
        throw std::runtime_error(std::string("GovSourceCatalog::loadFromYaml: ") + e.what());
    }
#else
    (void)yaml_content;
#endif
}

// ============================================================================
// Built-in catalog – Federal Germany (Bund)
// ============================================================================

void GovSourceCatalog::populateBuiltinBund() {
    auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };

    {
        GovDataSource s;
        s.id                   = "gesetze_im_internet";
        s.name                 = "Gesetze im Internet (BMJV)";
        s.type                 = GovSourceType::BUND;
        s.search_style         = GovSearchStyle::LISTING;
        s.language             = "de";
        s.base_url             = "https://www.gesetze-im-internet.de";
        s.search_url           = "https://www.gesetze-im-internet.de/Teilliste_A.html";
        s.sitemap_url          = "https://www.gesetze-im-internet.de/gii-toc.xml";
        s.search_param         = "giiIndex";
        s.page_param           = "p";
        s.result_list_selector = ".jnnormcontainer";
        s.notes                = "Official federal law texts; XML TOC available";
        add(s);
    }
    {
        GovDataSource s;
        s.id           = "rechtsprechung_im_internet";
        s.name         = "Rechtsprechung im Internet (BGH/BVerfG)";
        s.type         = GovSourceType::BUND;
        s.search_style = GovSearchStyle::HTML_FORM;
        s.language     = "de";
        s.base_url     = "https://www.rechtsprechung-im-internet.de";
        s.search_url   = "https://www.rechtsprechung-im-internet.de/jportal/portal/t/q5/page/"
                       "bsjrsprod.psml?doc.hl=1&action=portlets.jw.MainAction&showdoccase=1&paramfromHL=true";
        s.search_param         = "query";
        s.page_param           = "page";
        s.form_method          = "GET";
        s.result_list_selector = ".result-list-entry";
        s.next_page_selector   = "a.next-page";
        s.notes                = "Federal court decisions (BGH, BVerwG, BFH, BAG, BSG, BPatG)";
        add(s);
    }
    {
        GovDataSource s;
        s.id           = "bundesanzeiger";
        s.name         = "Bundesanzeiger";
        s.type         = GovSourceType::BUND;
        s.search_style = GovSearchStyle::HTML_FORM;
        s.language     = "de";
        s.base_url     = "https://www.bundesanzeiger.de";
        s.search_url
            = "https://www.bundesanzeiger.de/pub/de/start?0-2.IBackPageLink-backPageLink-wr-form=&detail.lang=de";
        s.search_param         = "fulltext";
        s.page_param           = "page";
        s.form_method          = "GET";
        s.result_list_selector = ".result_container";
        s.notes                = "Official Federal Gazette – Amtlicher Teil and Nichtamtlicher Teil";
        add(s);
    }
    {
        GovDataSource s;
        s.id                = "bundestag_dip";
        s.name              = "Bundestag – Dokumentations- und Informationssystem (DIP)";
        s.type              = GovSourceType::BUND;
        s.search_style      = GovSearchStyle::REST_JSON;
        s.language          = "de";
        s.base_url          = "https://search.dip.bundestag.de";
        s.search_url        = "https://search.dip.bundestag.de/api/v1/vorgang";
        s.api_endpoint      = "https://search.dip.bundestag.de/api/v1/vorgang";
        s.api_results_field = "documents";
        s.search_param      = "f.suche";
        s.page_param        = "cursor";
        s.api_cursor_field  = "cursor";
        s.api_key_env       = "BUNDESTAG_API_KEY";
        s.notes             = "Bundestag parliamentary documents REST API; requires free API key";
        add(s);
    }
    {
        GovDataSource s;
        s.id                   = "bundesrat";
        s.name                 = "Bundesrat – Drucksachen und Plenarprotokolle";
        s.type                 = GovSourceType::BUND;
        s.search_style         = GovSearchStyle::HTML_FORM;
        s.language             = "de";
        s.base_url             = "https://www.bundesrat.de";
        s.search_url           = "https://www.bundesrat.de/DE/Service/Suche/suche_node.html";
        s.search_param         = "query";
        s.page_param           = "p";
        s.form_method          = "GET";
        s.result_list_selector = ".result-item";
        s.notes                = "Federal Council plenary protocols and legislative documents";
        add(s);
    }
    {
        GovDataSource s;
        s.id                   = "openjur";
        s.name                 = "OpenJur – Freie Rechtsprechungsdatenbank";
        s.type                 = GovSourceType::BUND;
        s.search_style         = GovSearchStyle::HTML_FORM;
        s.language             = "de";
        s.base_url             = "https://openjur.de";
        s.search_url           = "https://openjur.de/suche/";
        s.search_param         = "q";
        s.page_param           = "p";
        s.form_method          = "GET";
        s.result_list_selector = ".result-list > li";
        s.next_page_selector   = "a[rel=next]";
        s.notes                = "Open-access German court decisions database (concrete reference site)";
        add(s);
    }
    {
        GovDataSource s;
        s.id                   = "dejure";
        s.name                 = "dejure.org – Rechtsprechung und Gesetze";
        s.type                 = GovSourceType::BUND;
        s.search_style         = GovSearchStyle::HTML_FORM;
        s.language             = "de";
        s.base_url             = "https://dejure.org";
        s.search_url           = "https://dejure.org/suche/";
        s.search_param         = "query";
        s.page_param           = "seite";
        s.form_method          = "GET";
        s.result_list_selector = "#ergebnis-liste li";
        s.next_page_selector   = "a.next";
        s.notes                = "dejure.org legal search with cross-references";
        add(s);
    }
    {
        GovDataSource s;
        s.id           = "bverfg";
        s.name         = "Bundesverfassungsgericht – Entscheidungen";
        s.type         = GovSourceType::BUND;
        s.search_style = GovSearchStyle::HTML_FORM;
        s.language     = "de";
        s.base_url     = "https://www.bundesverfassungsgericht.de";
        s.search_url
            = "https://www.bundesverfassungsgericht.de/SiteGlobals/Forms/Suche/Entscheidungssuche_Formular.html";
        s.search_param         = "gtp";
        s.page_param           = "s";
        s.form_method          = "GET";
        s.result_list_selector = ".c-result-list__item";
        s.notes                = "Federal Constitutional Court decisions";
        add(s);
    }
}

// ============================================================================
// Built-in catalog – 16 Bundesländer
// ============================================================================

void GovSourceCatalog::populateBuiltinBundeslaender() {
    auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };

    struct LandEntry {
        std::string id, name, iso, base_url, search_url, search_param, page_param, result_selector, notes;
        GovSearchStyle style = GovSearchStyle::HTML_FORM;
    };

    // Canonical list of all 16 Bundesländer law portals
    const LandEntry entries[] = {
        {"gesetze_bw", "Landesrecht Baden-Württemberg", "DE-BW", "https://www.landesrecht-bw.de",
         "https://www.landesrecht-bw.de/jportal/portal/t/h5/page/bsbawüprod.psml?action=portlets.jw.MainAction",
         "query", "page", ".result-list-entry", "Baden-Württemberg law and court decisions"},
        {"gesetze_by", "Bayerisches Recht Online", "DE-BY", "https://www.gesetze-bayern.de",
         "https://www.gesetze-bayern.de/Content/Document/search", "searchterm", "pagenumber", ".searchresult",
         "Bavarian state law portal"},
        {"gesetze_be", "Berliner Vorschriften- und Rechtsprechungsdatenbank", "DE-BE", "https://gesetze.berlin.de",
         "https://gesetze.berlin.de/bsbe/search", "query", "page", ".resultitem", "Berlin laws and court decisions"},
        {"gesetze_bb", "Landesrecht Brandenburg (BRAVORS)", "DE-BB", "https://bravors.brandenburg.de",
         "https://bravors.brandenburg.de/de/gesetze", "query", "seite", ".result", "Brandenburg state law"},
        {"gesetze_hb", "Bremisches Landesrecht", "DE-HB", "https://www.landesrecht.bremen.de",
         "https://www.landesrecht.bremen.de/bsbre/search", "query", "page", ".resultitem", "Bremen state law"},
        {"gesetze_hh", "Hamburgisches Recht", "DE-HH", "https://www.landesrecht-hamburg.de",
         "https://www.landesrecht-hamburg.de/bsham/search", "query", "page", ".resultitem", "Hamburg state law"},
        {"gesetze_he", "Hessisches Recht (HessenRecht)", "DE-HE", "https://www.rv.hessenrecht.hessen.de",
         "https://www.rv.hessenrecht.hessen.de/bshe/search", "query", "page", ".resultitem", "Hessian law portal"},
        {"gesetze_mv", "Landesrecht MV", "DE-MV", "https://www.landesrecht-mv.de",
         "https://www.landesrecht-mv.de/bsmv/search", "query", "page", ".resultitem",
         "Mecklenburg-Vorpommern state law"},
        {"gesetze_ni", "Niedersächsisches Recht (Voris)", "DE-NI", "https://www.nds-voris.de",
         "https://www.nds-voris.de/jportal/portal/t/bm/page/bsvorisprod.psml", "query", "page", ".result-list-entry",
         "Lower Saxony law"},
        {"gesetze_nw", "Recht NRW", "DE-NW", "https://recht.nrw.de",
         "https://recht.nrw.de/lmi/owa/pl_text_suche.show_search", "v_search_text", "v_seite", ".result",
         "North Rhine-Westphalia law portal"},
        {"gesetze_rp", "Landesrecht Rheinland-Pfalz", "DE-RP", "https://landesrecht.rlp.de",
         "https://landesrecht.rlp.de/bsrp/search", "query", "page", ".resultitem", "Rhineland-Palatinate state law"},
        {"gesetze_sl", "Saarländisches Recht", "DE-SL", "https://recht.saarland.de",
         "https://recht.saarland.de/bssl/search", "query", "page", ".resultitem", "Saarland state law"},
        {"gesetze_sn", "Sächsisches Recht (REVOSax)", "DE-SN", "https://www.revosax.sachsen.de",
         "https://www.revosax.sachsen.de/suche", "q", "seite", ".searchresult", "Saxony state law"},
        {"gesetze_st", "Landesrecht Sachsen-Anhalt", "DE-ST", "https://www.landesrecht.sachsen-anhalt.de",
         "https://www.landesrecht.sachsen-anhalt.de/bsst/search", "query", "page", ".resultitem",
         "Saxony-Anhalt state law"},
        {"gesetze_sh", "Rechtsprechungsdatenbank SH", "DE-SH", "https://www.gesetze-rechtsprechung.sh.juris.de",
         "https://www.gesetze-rechtsprechung.sh.juris.de/bssh/search", "query", "page", ".resultitem",
         "Schleswig-Holstein law portal"},
        {"gesetze_th", "Thüringer Landesrecht", "DE-TH", "https://landesrecht.thueringen.de",
         "https://landesrecht.thueringen.de/bsth/search", "query", "page", ".resultitem", "Thuringia state law"},
    };

    for (const auto &e : entries) {
        GovDataSource s;
        s.id                   = e.id;
        s.name                 = e.name;
        s.bundesland           = e.iso;
        s.type                 = GovSourceType::BUNDESLAND;
        s.search_style         = e.style;
        s.language             = "de";
        s.base_url             = e.base_url;
        s.search_url           = e.search_url;
        s.search_param         = e.search_param;
        s.page_param           = e.page_param;
        s.result_list_selector = e.result_selector;
        s.form_method          = "GET";
        s.next_page_selector   = "a.next, a[rel=next], .pager-next a";
        s.notes                = e.notes;
        add(s);
    }
}

// ============================================================================
// Built-in catalog – European Union
// ============================================================================

void GovSourceCatalog::populateBuiltinEU() {
    auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };

    {
        GovDataSource s;
        s.id                   = "eurlex";
        s.name                 = "EUR-Lex – EU Law and Case Law";
        s.type                 = GovSourceType::EU;
        s.search_style         = GovSearchStyle::HTML_FORM;
        s.language             = "de";
        s.base_url             = "https://eur-lex.europa.eu";
        s.search_url           = "https://eur-lex.europa.eu/search.html";
        s.search_param         = "text";
        s.page_param           = "page";
        s.form_method          = "GET";
        s.result_list_selector = ".SearchResult";
        s.next_page_selector   = "a.page-next";
        s.extra_params         = {{"scope", "EURLEX"}, {"type", "quick"}, {"lang", "de"}};
        s.sitemap_url          = "https://eur-lex.europa.eu/sitemap.xml";
        s.notes                = "Official EU law portal; HTML search form + CELLAR SPARQL available";
        add(s);
    }
    {
        GovDataSource s;
        s.id                = "eurlex_api";
        s.name              = "EUR-Lex REST/SPARQL API (CELLAR)";
        s.type              = GovSourceType::EU;
        s.search_style      = GovSearchStyle::EURLEX_API;
        s.language          = "de";
        s.base_url          = "https://publications.europa.eu";
        s.api_endpoint      = "https://publications.europa.eu/webapi/rdf/sparql";
        s.api_results_field = "results.bindings";
        s.notes             = "EUR-Lex CELLAR SPARQL endpoint for structured metadata queries";
        add(s);
    }
    {
        GovDataSource s;
        s.id                   = "curia";
        s.name                 = "Curia – Court of Justice of the EU";
        s.type                 = GovSourceType::EU;
        s.search_style         = GovSearchStyle::HTML_FORM;
        s.language             = "de";
        s.base_url             = "https://curia.europa.eu";
        s.search_url           = "https://curia.europa.eu/juris/recherche.jsf";
        s.search_param         = "query";
        s.page_param           = "page";
        s.form_method          = "POST";
        s.result_list_selector = ".rechercheresultat";
        s.next_page_selector   = "a.suivante";
        s.notes                = "CJEU and General Court judgments and opinions";
        add(s);
    }
    {
        GovDataSource s;
        s.id                = "europarl";
        s.name              = "Europäisches Parlament – Legislative Observatory";
        s.type              = GovSourceType::EU;
        s.search_style      = GovSearchStyle::REST_JSON;
        s.language          = "de";
        s.base_url          = "https://www.europarl.europa.eu";
        s.search_url        = "https://www.europarl.europa.eu/search/en/procedures/search";
        s.api_endpoint      = "https://www.europarl.europa.eu/search/en/procedures/search";
        s.api_results_field = "data";
        s.search_param      = "q";
        s.page_param        = "page";
        s.notes             = "European Parliament legislative procedures and documents";
        add(s);
    }
    {
        GovDataSource s;
        s.id                   = "eu_publications";
        s.name                 = "EU Publications Office (OP)";
        s.type                 = GovSourceType::EU;
        s.search_style         = GovSearchStyle::HTML_FORM;
        s.language             = "de";
        s.base_url             = "https://op.europa.eu";
        s.search_url           = "https://op.europa.eu/en/search-results";
        s.search_param         = "querytext";
        s.page_param           = "p";
        s.form_method          = "GET";
        s.result_list_selector = ".search-result-item";
        s.next_page_selector   = "a[rel=next]";
        s.notes                = "EU official publications including OJ C, L, and S series";
        add(s);
    }
}

} // namespace scraper
} // namespace themis
