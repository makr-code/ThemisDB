---
name: ♿ AI Review - Accessibility & Inclusive Design
about: Systematische Barrierefreiheits- und Inklusivitäts-Prüfung / Systematic accessibility and inclusive design review
title: '[ACCESSIBILITY-REVIEW] '
labels: ['type:systematic-review', 'area:accessibility', 'area:ux', 'needs-triage']
assignees: ''
---

<!-- 
Wiederholbare Template für Accessibility & Inclusive Design Reviews
Repeatable template for accessibility and inclusive design reviews
Empfohlene Häufigkeit: Vor Releases / Recommended frequency: Before releases
-->

## 🎯 Scope / Umfang

**Review Scope:** <!-- z.B. Web UI, API Documentation, CLI Tools -->
**Review Period:** <!-- z.B. Q1 2026, Pre-Release 1.4.0 -->
**Reviewer(s):** <!-- Namen der Reviewer -->
**Previous Review:** <!-- Datum des letzten Reviews -->

---

## 📊 WCAG Compliance / WCAG-Konformität

### WCAG Version & Level / WCAG-Version & Level
- **Target WCAG Version:** <!-- 2.1, 2.2 -->
- **Target Conformance Level:** <!-- A, AA, AAA -->
- **Current Conformance Level:** <!-- A, AA, AAA, Non-compliant -->

### WCAG 2.1/2.2 Principles / Prinzipien

#### 1. Perceivable / Wahrnehmbar
- [ ] **1.1 Text Alternatives** - Alt text for images, icons
- [ ] **1.2 Time-based Media** - Captions, transcripts for video/audio
- [ ] **1.3 Adaptable** - Content can be presented in different ways
- [ ] **1.4 Distinguishable** - Easy to see and hear content

**Issues Found:**


#### 2. Operable / Bedienbar
- [ ] **2.1 Keyboard Accessible** - All functionality via keyboard
- [ ] **2.2 Enough Time** - Users have enough time to read/use content
- [ ] **2.3 Seizures** - No content that causes seizures
- [ ] **2.4 Navigable** - Ways to navigate, find content, and locate
- [ ] **2.5 Input Modalities** - Easy to operate with various inputs

**Issues Found:**


#### 3. Understandable / Verständlich
- [ ] **3.1 Readable** - Text content is readable and understandable
- [ ] **3.2 Predictable** - Web pages appear and operate predictably
- [ ] **3.3 Input Assistance** - Help users avoid and correct mistakes

**Issues Found:**


#### 4. Robust / Robust
- [ ] **4.1 Compatible** - Compatible with current and future tools

**Issues Found:**


---

## 🖥️ Web UI Accessibility / Web-UI-Barrierefreiheit

### Semantic HTML / Semantisches HTML
- [ ] **Proper heading hierarchy** (h1, h2, h3, etc.)
- [ ] **Semantic elements** used (nav, main, article, aside, footer)
- [ ] **Landmark regions** defined
- [ ] **Lists** used appropriately (ul, ol, dl)
- [ ] **Tables** have proper structure (thead, tbody, th, caption)

**Semantic Issues:**


### Keyboard Navigation / Tastatur-Navigation
- [ ] **All interactive elements** keyboard accessible
- [ ] **Tab order** logical and intuitive
- [ ] **Focus indicators** visible
- [ ] **Skip links** provided
- [ ] **No keyboard traps**
- [ ] **Keyboard shortcuts** documented

**Keyboard Navigation Issues:**


### Screen Reader Support / Screen-Reader-Unterstützung
- [ ] **ARIA labels** used appropriately
- [ ] **ARIA roles** correctly assigned
- [ ] **ARIA live regions** for dynamic content
- [ ] **Alternative text** for images
- [ ] **Form labels** associated with inputs
- [ ] **Error messages** announced

**Tested with:**
- [ ] NVDA (Windows)
- [ ] JAWS (Windows)
- [ ] VoiceOver (macOS/iOS)
- [ ] TalkBack (Android)

**Screen Reader Issues:**


### Color & Contrast / Farbe & Kontrast
- [ ] **Text contrast ratio** ≥ 4.5:1 (normal text)
- [ ] **Large text contrast ratio** ≥ 3:1
- [ ] **UI components contrast** ≥ 3:1
- [ ] **Color not sole indicator** of information
- [ ] **Link text** distinguishable from body text

**Contrast Issues:**


### Forms & Input / Formulare & Eingabe
- [ ] **Labels** for all form fields
- [ ] **Required fields** clearly indicated
- [ ] **Error messages** clear and helpful
- [ ] **Error identification** programmatic
- [ ] **Input suggestions** provided
- [ ] **Autocomplete** attributes used

**Form Issues:**


---

## 📱 Mobile Accessibility / Mobile-Barrierefreiheit

### Touch Targets / Touch-Ziele
- [ ] **Touch target size** ≥ 44x44 pixels
- [ ] **Spacing between targets** ≥ 8 pixels
- [ ] **No overlapping targets**

### Mobile-Specific / Mobilspezifisch
- [ ] **Screen orientation** not restricted
- [ ] **Zoom** not disabled (max-scale not < 2)
- [ ] **Gestures** not required
- [ ] **Alternative inputs** for motion/shake

**Mobile Issues:**


---

## 🔧 API & Developer Tools Accessibility / API-Barrierefreiheit

### API Documentation / API-Dokumentation
- [ ] **Code examples** accessible (proper formatting, descriptions)
- [ ] **Error codes** well-documented
- [ ] **API responses** include meaningful messages
- [ ] **SDKs** follow accessibility best practices

### CLI Tools / CLI-Werkzeuge
- [ ] **Screen reader compatible** output
- [ ] **Color not sole indicator**
- [ ] **Progress indicators** accessible
- [ ] **Help text** comprehensive

**API/CLI Issues:**


---

## 🌍 Internationalization (i18n) & Accessibility / Internationalisierung

### Language Support / Sprachunterstützung
- [ ] **lang attribute** set correctly
- [ ] **Language changes** marked
- [ ] **Text direction** supported (LTR, RTL)
- [ ] **Character encoding** UTF-8

### Cultural Considerations / Kulturelle Aspekte
- [ ] **Date/time formats** localized
- [ ] **Number formats** localized
- [ ] **Icons** culturally appropriate
- [ ] **Images** culturally sensitive

**i18n Issues:**


---

## 🧪 Testing / Testing

### Automated Testing / Automatisierte Tests
- **Testing Tools:**
  - [ ] axe DevTools
  - [ ] WAVE
  - [ ] Lighthouse
  - [ ] pa11y
  - [ ] Other: _______

**Automated Test Results:**
- **Total Issues:** 
- **Critical:** 
- **Serious:** 
- **Moderate:** 
- **Minor:** 

### Manual Testing / Manuelle Tests
- [ ] **Keyboard-only** navigation test
- [ ] **Screen reader** test
- [ ] **Zoom test** (up to 200%)
- [ ] **Color blindness** simulation
- [ ] **High contrast mode** test

### User Testing / Benutzer-Tests
- [ ] **Testing with users with disabilities**
- [ ] **Assistive technology** users involved
- [ ] **Feedback** collected and documented

**User Testing Findings:**


---

## 📋 Accessibility Statement / Barrierefreiheits-Erklärung

### Statement Status / Erklärung-Status
- [ ] **Accessibility statement** published
- [ ] **Conformance level** declared
- [ ] **Known limitations** listed
- [ ] **Contact information** for feedback
- [ ] **Last updated** date current

**Statement Location:** <!-- URL -->

---

## 🎨 Design System / Design-System

### Accessible Components / Barrierefreie Komponenten
- [ ] **Buttons** accessible
- [ ] **Modals/Dialogs** accessible
- [ ] **Dropdowns/Selects** accessible
- [ ] **Tabs** accessible
- [ ] **Tooltips** accessible
- [ ] **Notifications/Alerts** accessible
- [ ] **Data tables** accessible
- [ ] **Charts/Graphs** accessible (with text alternatives)

**Component Issues:**


---

## 🔍 Common Issues / Häufige Probleme

### Critical Issues / Kritische Probleme
1. **Issue 1:**
   - Severity: <!-- Critical/High/Medium/Low -->
   - WCAG Criterion: 
   - Impact: 
   - Remediation: 

2. **Issue 2:**
   - Severity: 
   - WCAG Criterion: 
   - Impact: 
   - Remediation: 

### Quick Wins / Schnelle Erfolge
Low-effort, high-impact fixes:
1. 
2. 
3. 

---

## 📚 Training & Awareness / Schulung & Bewusstsein

### Team Knowledge / Team-Wissen
- [ ] **Accessibility training** completed
- [ ] **WCAG guidelines** understood
- [ ] **Testing tools** familiar
- [ ] **Best practices** documented

### Design & Development Process / Design & Entwicklung
- [ ] **Accessibility in design phase**
- [ ] **Accessibility in code reviews**
- [ ] **Accessibility testing** in QA
- [ ] **Accessibility regression** testing

**Training Gaps:**


---

## 🗺️ Roadmap / Roadmap

### Short-Term (Next 3 Months)
- [ ] Fix critical accessibility issues
- [ ] Implement keyboard navigation improvements
- [ ] Add ARIA labels where missing
- [ ] 

### Medium-Term (3-6 Months)
- [ ] Achieve WCAG 2.1 Level AA compliance
- [ ] Conduct user testing with assistive tech users
- [ ] Publish accessibility statement
- [ ] 

### Long-Term (6-12 Months)
- [ ] Achieve WCAG 2.2 Level AA compliance
- [ ] Implement accessibility monitoring
- [ ] Establish accessibility champions program
- [ ] 

---

## ✅ Action Items / Aktionspunkte

### Critical (P0) - Blocking Issues
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - WCAG Criterion: 
   - Description: 

### High Priority (P1)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - WCAG Criterion: 
   - Description: 

2. [ ] **Action 2:**
   - Owner: 
   - Due Date: 
   - WCAG Criterion: 
   - Description: 

### Medium Priority (P2)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - WCAG Criterion: 
   - Description: 

---

## 📚 References / Referenzen

### Internal Documentation
- [Accessibility Guidelines](docs/accessibility/)
- [Design System](docs/design-system/)
- [UI Components](docs/ui-components/)

### External Resources
- [WCAG 2.1 Guidelines](https://www.w3.org/WAI/WCAG21/quickref/)
- [WCAG 2.2 Guidelines](https://www.w3.org/WAI/WCAG22/quickref/)
- [WAI-ARIA Authoring Practices](https://www.w3.org/WAI/ARIA/apg/)
- [WebAIM Resources](https://webaim.org/resources/)
- [A11y Project](https://www.a11yproject.com/)
- [Inclusive Components](https://inclusive-components.design/)

### Testing Tools
- [axe DevTools](https://www.deque.com/axe/devtools/)
- [WAVE](https://wave.webaim.org/)
- [Lighthouse](https://developers.google.com/web/tools/lighthouse)
- [pa11y](https://pa11y.org/)

---

## 📋 Review Checklist / Review-Checkliste

- [ ] WCAG compliance assessed
- [ ] Keyboard navigation tested
- [ ] Screen reader compatibility verified
- [ ] Color contrast checked
- [ ] Forms and inputs reviewed
- [ ] Mobile accessibility tested
- [ ] API and CLI tools reviewed
- [ ] Automated testing completed
- [ ] Manual testing performed
- [ ] User testing conducted (if applicable)
- [ ] Accessibility statement reviewed
- [ ] Action items created and assigned
- [ ] Sign-offs obtained from UX and development teams

---

**Review Date:** <!-- YYYY-MM-DD -->
**Next Review:** <!-- YYYY-MM-DD (empfohlen: vor jedem Release) -->
**Sign-Off:** <!-- UX Lead, Accessibility Specialist, Development Lead -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB UX Team
