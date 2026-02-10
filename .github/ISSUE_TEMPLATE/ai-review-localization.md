---
name: 🌍 AI Review - Localization & Internationalization (i18n)
about: Systematische Lokalisierungs- und Internationalisierungs-Prüfung / Systematic localization and internationalization review
title: '[i18n-REVIEW] '
labels: ['type:systematic-review', 'area:i18n', 'area:localization', 'needs-triage']
assignees: ''
---

<!-- 
Wiederholbare Template für Localization & i18n Reviews
Repeatable template for localization and internationalization reviews
Empfohlene Häufigkeit: Halbjährlich / Recommended frequency: Bi-annually
-->

## 🎯 Scope / Umfang

**Review Scope:** <!-- z.B. Full application, Specific module, Documentation -->
**Review Period:** <!-- z.B. Q1 2026, Version 1.4.x -->
**Reviewer(s):** <!-- Namen der Reviewer -->
**Previous Review:** <!-- Datum des letzten Reviews -->

---

## 🌐 Supported Languages / Unterstützte Sprachen

### Current Language Support / Aktuelle Sprachunterstützung
- **Total Languages:** 
- **Fully Translated:** 
- **Partially Translated:** 
- **In Progress:** 

**Language List:**
| Language | Code | Coverage | Status |
|----------|------|----------|--------|
| English | en | 100% | ✅ Complete |
| German | de | % | |
| French | fr | % | |
| Spanish | es | % | |
| Japanese | ja | % | |
| Chinese (Simplified) | zh-CN | % | |
| Other | | % | |

---

## 🔤 String Externalization / String-Externalisierung

### String Hardcoding / Hardcodierte Strings
- **Total Strings:** 
- **Externalized Strings:** <!-- % -->
- **Hardcoded Strings:** <!-- Count -->
- **Untranslatable Strings:** <!-- Technical terms, etc. -->

**Hardcoded String Locations:**
1. 
2. 
3. 

### Translation Keys / Übersetzungs-Schlüssel
- [ ] **Consistent naming** convention
- [ ] **Hierarchical structure** (e.g., `component.action.label`)
- [ ] **No duplicate keys**
- [ ] **Context provided** for translators
- [ ] **Pluralization** handled
- [ ] **Gender** considerations

**Key Convention:** <!-- e.g., namespace.component.element.purpose -->

**Translation Key Issues:**


---

## 📝 Translation Quality / Übersetzungs-Qualität

### Translation Status / Übersetzungs-Status
- **Translation Memory (TM)** used: <!-- Yes/No -->
- **Glossary** maintained: <!-- Yes/No -->
- **Style guide** available: <!-- Yes/No -->
- **Professional translators**: <!-- Yes/No, Language -->
- **Community translations**: <!-- Yes/No -->

### Translation Issues / Übersetzungs-Probleme
- [ ] **Missing translations**
- [ ] **Incorrect translations**
- [ ] **Inconsistent terminology**
- [ ] **Cultural inappropriateness**
- [ ] **Truncated text**
- [ ] **Overlapping text**

**Critical Translation Issues:**
1. **Language:**
   - Location: 
   - Issue: 
   - Correct Translation: 

2. **Language:**
   - Location: 
   - Issue: 
   - Correct Translation: 

---

## 🗓️ Date, Time & Number Formatting / Datums-, Zeit- & Zahlenformatierung

### Date & Time / Datum & Zeit
- [ ] **Date formats** localized (DD/MM/YYYY, MM/DD/YYYY, YYYY-MM-DD)
- [ ] **Time formats** localized (12h, 24h)
- [ ] **Timezone** handling correct
- [ ] **Calendar systems** supported (Gregorian, Islamic, etc.)
- [ ] **Relative time** (e.g., "2 hours ago") localized

**Date/Time Formatting Library:** <!-- Moment.js, date-fns, Intl, etc. -->

**Date/Time Issues:**


### Number & Currency / Zahlen & Währung
- [ ] **Number separators** localized (1,000.00 vs 1.000,00)
- [ ] **Currency symbols** correct
- [ ] **Currency formatting** localized
- [ ] **Percentage formatting** localized
- [ ] **Large numbers** formatted (K, M, B)

**Number/Currency Issues:**


---

## 🎭 Text Direction & Layout / Textrichtung & Layout

### RTL (Right-to-Left) Support / RTL-Unterstützung
- **RTL Languages Supported:** <!-- Arabic, Hebrew, etc. -->
- [ ] **UI mirrored** for RTL
- [ ] **Text alignment** adjusted
- [ ] **Icons** mirrored appropriately
- [ ] **Bidirectional text** handled (BiDi)
- [ ] **CSS logical properties** used

**RTL Languages:** 
- [ ] Arabic (ar)
- [ ] Hebrew (he)
- [ ] Persian (fa)
- [ ] Urdu (ur)

**RTL Issues:**


### Layout Flexibility / Layout-Flexibilität
- [ ] **Variable text length** accommodated
- [ ] **No fixed widths** for text containers
- [ ] **Overflow** handled gracefully
- [ ] **Line breaks** appropriate
- [ ] **Hyphenation** rules applied

**Layout Issues:**


---

## 🔠 Character Encoding & Fonts / Zeichenkodierung & Schriftarten

### Character Encoding / Zeichenkodierung
- **Encoding:** <!-- UTF-8, UTF-16, etc. -->
- [ ] **UTF-8** used throughout
- [ ] **Special characters** supported
- [ ] **Emoji** support
- [ ] **Diacritics** supported
- [ ] **CJK characters** supported

### Font Support / Schriftarten-Unterstützung
- [ ] **Font stacks** include international fonts
- [ ] **Fallback fonts** appropriate
- [ ] **Font loading** optimized
- [ ] **Web fonts** for all scripts

**Font Stack:** <!-- Example: -apple-system, BlinkMacSystemFont, 'Noto Sans', sans-serif -->

**Font Issues:**


---

## 💬 Content Localization / Inhalts-Lokalisierung

### Images & Media / Bilder & Medien
- [ ] **Images** with text localized
- [ ] **Icons** culturally appropriate
- [ ] **Videos** with subtitles/captions
- [ ] **Audio** with transcripts
- [ ] **Culturally sensitive** content reviewed

**Media Issues:**


### UI Text / UI-Text
- [ ] **Button labels** translated
- [ ] **Error messages** translated
- [ ] **Help text** translated
- [ ] **Tooltips** translated
- [ ] **Placeholder text** translated
- [ ] **Alt text** translated

### Documentation / Dokumentation
- [ ] **User guides** translated
- [ ] **API documentation** translated
- [ ] **Release notes** translated
- [ ] **FAQs** translated
- [ ] **Legal documents** (Terms, Privacy) translated

**Documentation Translation Status:**
| Document | Languages | % Complete |
|----------|-----------|------------|
| User Guide | | |
| API Docs | | |
| FAQ | | |

---

## 🌏 Regional & Cultural Considerations / Regionale & kulturelle Aspekte

### Regional Differences / Regionale Unterschiede
- [ ] **Address formats** localized
- [ ] **Phone number formats** localized
- [ ] **Postal code formats** validated
- [ ] **Name formats** considered (first/last, order)
- [ ] **Honorifics** supported

### Cultural Sensitivity / Kulturelle Sensibilität
- [ ] **Colors** culturally appropriate
- [ ] **Symbols** culturally appropriate
- [ ] **Gestures/hand signs** reviewed
- [ ] **Religious considerations** addressed
- [ ] **Political sensitivities** avoided

**Cultural Issues:**


---

## 🔧 Technical Implementation / Technische Implementierung

### i18n Framework / i18n-Framework
- **Framework:** <!-- React-intl, i18next, Gettext, etc. -->
- **Version:** 
- [ ] **ICU message format** supported
- [ ] **Pluralization** rules implemented
- [ ] **Gender** handling
- [ ] **Context** support

### Language Detection / Spracherkennung
- [ ] **Browser language** detected
- [ ] **User preference** saved
- [ ] **URL parameter** support (?lang=de)
- [ ] **Subdomain** support (de.example.com)
- [ ] **Cookie/localStorage** persistence

### Dynamic Content / Dynamischer Inhalt
- [ ] **Database content** translatable
- [ ] **User-generated content** language-tagged
- [ ] **Mixed-language** content handled
- [ ] **Translation API** integrated

**i18n Implementation Issues:**


---

## 📊 Translation Workflow / Übersetzungs-Workflow

### Translation Management / Übersetzungs-Management
- **TMS (Translation Management System):** <!-- Crowdin, Phrase, Lokalise, etc. -->
- [ ] **Translation workflow** defined
- [ ] **Translator access** managed
- [ ] **Review process** established
- [ ] **QA process** in place

### Translation Tools / Übersetzungs-Tools
- [ ] **Translation Memory** (TM) system
- [ ] **Terminology database**
- [ ] **Machine translation** (MT) integration
- [ ] **Context screenshots** provided
- [ ] **Pseudo-localization** testing

**Workflow Issues:**


---

## 🧪 Testing / Testing

### Localization Testing / Lokalisierungs-Tests
- [ ] **Visual testing** (all languages)
- [ ] **Functional testing** (all languages)
- [ ] **Linguistic testing** (native speakers)
- [ ] **Pseudo-localization** testing
- [ ] **RTL testing**

### Automated Testing / Automatisierte Tests
- [ ] **Missing translation** detection
- [ ] **Unused translation** detection
- [ ] **Translation key** validation
- [ ] **Character encoding** tests
- [ ] **String length** tests

**Testing Issues:**


---

## 📈 Localization Metrics / Lokalisierungs-Metriken

### Coverage Metrics / Abdeckungs-Metriken
- **Overall Translation Coverage:** <!-- % -->
- **UI Coverage:** <!-- % -->
- **Documentation Coverage:** <!-- % -->
- **Help Content Coverage:** <!-- % -->

### Quality Metrics / Qualitäts-Metriken
- **Translation Accuracy:** <!-- Reviewed/Total -->
- **Native Speaker Reviews:** <!-- Count -->
- **User Feedback:** <!-- Positive/Negative -->

---

## 🗺️ Roadmap / Roadmap

### Short-Term (Next 3 Months)
- [ ] Complete existing language translations
- [ ] Fix critical localization issues
- [ ] Implement missing i18n features
- [ ] 

### Medium-Term (3-6 Months)
- [ ] Add 2-3 new languages
- [ ] Implement translation workflow automation
- [ ] Improve RTL support
- [ ] 

### Long-Term (6-12 Months)
- [ ] Support 10+ languages
- [ ] Establish community translation program
- [ ] Implement advanced i18n features
- [ ] 

---

## ✅ Action Items / Aktionspunkte

### Critical (P0)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Language: 
   - Description: 

### High Priority (P1)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Language: 
   - Description: 

2. [ ] **Action 2:**
   - Owner: 
   - Due Date: 
   - Language: 
   - Description: 

### Medium Priority (P2)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Language: 
   - Description: 

---

## 📚 References / Referenzen

### Internal Documentation
- [i18n Guidelines](docs/i18n/)
- [Translation Workflow](docs/translation/)
- [Style Guide](docs/style-guide/)

### External Resources
- [W3C Internationalization](https://www.w3.org/International/)
- [Unicode CLDR](http://cldr.unicode.org/)
- [ICU Message Format](https://unicode-org.github.io/icu/userguide/format_parse/messages/)
- [i18next Documentation](https://www.i18next.com/)
- [React-Intl Documentation](https://formatjs.io/docs/react-intl/)

### Tools
- [Crowdin](https://crowdin.com/)
- [Lokalise](https://lokalise.com/)
- [Phrase](https://phrase.com/)
- [Weblate](https://weblate.org/)

---

## 📋 Review Checklist / Review-Checkliste

- [ ] Language coverage assessed
- [ ] String externalization verified
- [ ] Translation quality reviewed
- [ ] Date/time/number formatting checked
- [ ] RTL support tested
- [ ] Character encoding verified
- [ ] Content localization reviewed
- [ ] Cultural considerations addressed
- [ ] Technical implementation assessed
- [ ] Translation workflow evaluated
- [ ] Testing completed
- [ ] Metrics collected
- [ ] Action items created and assigned
- [ ] Sign-offs obtained from i18n and product teams

---

**Review Date:** <!-- YYYY-MM-DD -->
**Next Review:** <!-- YYYY-MM-DD (empfohlen: +6 Monate) -->
**Sign-Off:** <!-- i18n Lead, Product Manager, Translation Team -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB i18n Team
