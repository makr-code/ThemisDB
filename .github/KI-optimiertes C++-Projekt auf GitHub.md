# **Architekturen der KI-gestützten C++-Entwicklung: Strategien zur Optimierung von GitHub-Repositories für Copilot und automatisierte Verifizierung**

Die Softwareentwicklung im Bereich der Systemprogrammierung, insbesondere mit C++, durchläuft derzeit eine fundamentale Transformation durch die Integration großskaliger Sprachmodelle (LLMs) direkt in den Entwicklungszyklus. Während traditionelle integrierte Entwicklungsumgebungen (IDEs) primär als Werkzeuge zur Code-Manipulation und Kompilierung dienten, fungieren sie heute als Schnittstellen für eine hybride Intelligenz, bei der GitHub Copilot als pair-programmer agiert. Die Effektivität dieses Systems hängt jedoch nicht allein von der Leistungsfähigkeit des zugrunde liegenden Modells ab, sondern maßgeblich von der Qualität und Struktur des bereitgestellten Kontexts. Ein optimal konfiguriertes C++-Projekt auf GitHub muss daher über die reine Quellcode-Verwaltung hinausgehen und eine semantische Brücke schlagen, die es der Künstlichen Intelligenz ermöglicht, architektonische Absichten, Build-Konfigurationen und Sicherheitsanforderungen präzise zu interpretieren.1

## **Die Architektur des semantischen Kontexts im C++-Ökosystem**

Das Fundament einer erfolgreichen KI-Integration liegt in der Schaffung eines "Semantic Context Layer". C++ stellt hierbei besondere Herausforderungen dar, da die Trennung zwischen Deklaration (Header-Dateien) und Definition (Source-Dateien) sowie die komplexe Auflösung von Abhängigkeiten über Build-Systeme wie CMake eine hohe kognitive Last für KI-Modelle erzeugen.1 GitHub Copilot operiert innerhalb eines begrenzten Kontextfensters, in dem es versucht, die wahrscheinlichsten nächsten Token basierend auf den aktuell geöffneten Dateien und der Workspace-Struktur vorherzusagen.1 Wenn dieser Kontext fragmentiert oder mehrdeutig ist, steigt die Wahrscheinlichkeit von Halluzinationen – also der Generierung von syntaktisch korrektem, aber logisch fehlerhaftem oder sicherheitstechnisch bedenklichem Code.1

### **Strukturierung des Repositories für maximale KI-Resonanz**

Ein modernes C++-Repository sollte so strukturiert sein, dass es die "Contextual Discovery" der KI aktiv unterstützt. Dies bedeutet die Implementierung spezifischer Verzeichnisse, die über die Standard-ISO-Strukturen hinausgehen. Das Verzeichnis ai\_context/ dient als persistenter Speicher für Referenzdokumente, die architektonische Entscheidungen (Architecture Decision Records, ADRs) und externe API-Dokumentationen enthalten, welche nicht direkt im Code abgebildet sind.6 Ergänzend dazu ermöglicht ein ai\_working/-Ordner der KI einen geschützten Raum für die Planung von Feature-Iterationen und das Debugging, ohne die Hauptquellcode-Basis durch temporäre Entwürfe zu korrumpieren.6

| Verzeichniskomponente | Zweck der KI-Optimierung | Beispielhafte Artefakte |
| :---- | :---- | :---- |
| .github/instructions/ | Lokale Steuerung der KI-Agenten | cpp-tools.instructions.md, ci-logic.instructions.md |
| ai\_context/ | Wissensbasis für domänenspezifische Logik | memory\_management\_policy.md, api\_v2\_spec.json |
| ai\_working/ | Iterativer Entwurfsraum für Agenten | refactoring\_plan.md, debug\_trace\_analysis.txt |
| .vscode/ | IDE-Kontext-Synchronisation | settings.json, tasks.json, launch.json |
| .claude/ / .gemini/ | Konfiguration für spezialisierte CLI-Agenten | custom\_commands.toml, project\_prompts.yml |

Diese Verzeichnisstruktur stellt sicher, dass KI-Agenten wie der Copilot Plan-Agent oder spezialisierte Modernisierungs-Agenten nicht nur den Ist-Zustand des Codes analysieren, sondern auch die zugrunde liegende Philosophie und die geplanten Evolutionsschritte verstehen.6

## **Konfiguration der Entwicklungsumgebung in Visual Studio Code**

Visual Studio Code hat sich als primäre Plattform für die KI-gestützte C++-Entwicklung etabliert, da es durch Erweiterungen wie das C/C++ Extension Pack und CMake Tools eine tiefe Integration von Sprachservern und KI-Modellen ermöglicht.2 Die optimale Konfiguration der Datei .vscode/settings.json ist dabei der erste Schritt, um Copilot den Zugriff auf tiefere semantische Informationen zu gewähren.

### **Aktivierung der C++ Code Editing Tools**

Ein entscheidender Durchbruch in der Präzision von GitHub Copilot für C++ ist die Nutzung von Symbol-Kontext-Werkzeugen. Durch die Aktivierung der Einstellung Enable Cpp Code Editing Tools erhält Copilot Zugriff auf spezialisierte Funktionen des C++-Sprachservers.2 Diese Werkzeuge erlauben es der KI, anstatt einer reinen Textsuche eine semantische Analyse der Codebasis durchzuführen.

Die Analyse von Symbolen umfasst drei Kernwerkzeuge:

1. GetSymbolInfo\_CppTools: Liefert detaillierte Informationen über Definitionen, Typen und Dokumentationen von Funktionen oder Klassen.2  
2. GetSymbolReferences\_CppTools: Identifiziert alle Verwendungen eines Symbols über das gesamte Projekt hinweg, was für sichere Refactorings unerlässlich ist.2  
3. GetSymbolCallHierarchy\_CppTools: Analysiert eingehende und ausgehende Aufrufe, um komplexe Abhängigkeitsketten und die potenziellen Auswirkungen von Änderungen zu verstehen.2

Die konsequente Nutzung dieser Tools reduziert die kognitive Last der KI und minimiert das Risiko von Fehlern bei der Bearbeitung großer, gewachsener Codebasen.9 Ein wesentlicher Best-Practice-Punkt hierbei ist die Verwendung absoluter Pfade bei der Interaktion mit diesen Tools, um Auflösungsfehler in komplexen Workspace-Strukturen zu vermeiden.10

### **Optimierung von Build- und Test-Tasks**

Damit GitHub Copilot Code nicht nur generieren, sondern auch verifizieren kann, muss die Build-Infrastruktur nahtlos integriert sein. Die Verwendung von CMake-Presets in Kombination mit .vscode/tasks.json ermöglicht es Copilot Chat, Projekte aktiv zu konfigurieren, zu bauen und zu testen.2 Anstatt dass der Entwickler manuell Build-Befehle eingeben muss, kann er Copilot anweisen: "Fixe den Kompilierfehler in Modul X und verifiziere dies mit einem Build".9 Copilot nutzt dann die definierten CMake-Tools, um die exakte Konfiguration des Entwicklers zu replizieren, anstatt auf ad-hoc Kommandozeilenaufrufe zurückzugreifen.8

| Task-Typ | JSON-Konfiguration (Beispiel) | KI-Relevanz |
| :---- | :---- | :---- |
| cmake: configure | {"type": "cmake", "command": "configure"} | Ermöglicht der KI das Auflösen von Abhängigkeiten |
| cmake: build | {"type": "cmake", "command": "build", "target": "all"} | Verifiziert die syntaktische Korrektheit generierten Codes |
| cmake: test | {"type": "cmake", "command": "test"} | Garantiert die funktionale Integrität nach Änderungen |

## **Custom Instructions: Das Regelwerk der KI-Interaktion**

Die Steuerung des KI-Verhaltens erfolgt maßgeblich über "Custom Instructions". Diese werden in einer zentralen Datei .github/copilot-instructions.md hinterlegt und bei jeder Interaktion als System-Prompt geladen.12 Für C++-Projekte ist dies das mächtigste Werkzeug, um die Einhaltung von Codierungsstandards und Sicherheitsrichtlinien zu erzwingen.13

### **Mandatierung von Sprachwerkzeugen und Standards**

In den Instruktionen sollte explizit festgelegt werden, dass Copilot bei der Suche nach Symbolverwendungen zwingend die C++-Sprachwerkzeuge anstatt einer einfachen Textsuche nutzen muss.10 Dies ist besonders bei Projekten mit vielen überladenen Funktionen oder komplexen Template-Strukturen kritisch, da eine textbasierte Suche (wie grep) hier oft unvollständige Ergebnisse liefert.

Zudem sollten architektonische Grundsätze wie RAII (Resource Acquisition Is Initialization) als obligatorisch definiert werden.14 Die Instruktionen können die KI anweisen, stets Smart-Pointer (std::unique\_ptr, std::shared\_ptr) anstelle von rohen Pointern zu verwenden und bei der Ressourcenverwaltung auf deterministische Destruktoren zu setzen.14 Durch die Integration von Beispielen für "guten" und "schlechten" Code in die Instruktionsdatei wird die Generierung konsistenterer Ergebnisse gefördert.12

### **Spezifische Richtlinien für Modern C++ (C++20/23)**

Da KI-Modelle oft auf älteren Code-Mustern trainiert wurden, müssen Instruktionen sie aktiv zu modernen Standards führen. Dies umfasst den Vorzug von:

* std::string\_view und std::span für effiziente Parameterübergabe ohne Kopien.14  
* Concepts zur Einschränkung von Templates, was der KI hilft, präzisere Vorschläge zu machen.14  
* Ranges-Bibliothek für deklarative Datenverarbeitung.14  
* Coroutinen für asynchrone Programmierung, wobei die KI hierbei besonders detaillierte Anweisungen zur Implementierung des Promise-Typs benötigt.16

## **Agentic Development: Spezialisierte KI-Agenten im Workflow**

Der Übergang von der einfachen Code-Vervollständigung hin zu autonom agierenden Agenten markiert die nächste Stufe der Produktivität. In VS Code können verschiedene Agenten für spezialisierte Aufgaben eingesetzt werden.

### **Der Modernisierungs-Agent (@Modernize)**

Speziell für C++-Legacy-Codebasen bietet der @Modernize-Agent die Möglichkeit, veraltete MSVC-Build-Tools oder veraltete Sprachkonstrukte systematisch zu aktualisieren.18 Der Prozess beginnt mit einer Assessment-Phase, in der der Agent eine assessment.md-Datei erstellt, welche Probleme und deren Schweregrad dokumentiert.18 Nach der Zustimmung des Entwicklers zu einem plan.md führt der Agent die Änderungen autonom durch und verifiziert sie durch Builds.18

### **Optimierung der Build-Performance (@BuildPerfCpp)**

Für große C++-Projekte, insbesondere in der Spieleentwicklung, ist die Build-Zeit ein kritischer Faktor. Der @BuildPerfCpp-Agent analysiert Build-Insights (basierend auf ETL-Traces) und identifiziert Engpässe wie teure Header-Inklusionen, langsame Template-Instanziierungen oder ineffiziente Funktionsgenerierungen.19 Durch die Fokussierung auf einzelne CMake-Targets oder MSBuild-Projekte können Optimierungsvorschläge gezielt und schnell erarbeitet werden.19

## **CI/CD-Automatisierung und Verifizierung durch GitHub Actions**

Trotz aller Optimierungen bleibt die Erkenntnis bestehen: KI-generierter Code muss wie der Code eines Junior-Entwicklers behandelt werden – er erfordert strenge Prüfung und automatisierte Verifizierung.3 Die CI/CD-Pipeline in GitHub Actions fungiert hierbei als unerbittliches Sicherheitsnetz.

### **Statische Analyse und Sicherheits-Scanning**

Ein robuster Workflow für C++ integriert mehrere Ebenen der Analyse. clang-tidy und clang-format stellen die Einhaltung von Stilvorgaben und grundlegenden logischen Mustern sicher.21 Die Ergebnisse dieser Tools sollten im SARIF-Format (Static Analysis Results Interchange Format) ausgegeben werden, damit sie nahtlos in die GitHub-Oberfläche integriert werden können.22

| Analyse-Ebene | Tool-Kombination | Zielsetzung |
| :---- | :---- | :---- |
| Stil & Format | clang-format | Konsistente Lesbarkeit für Mensch und KI |
| Semantisches Linting | clang-tidy | Vermeidung von "Common Pitfalls" (z.B. fehlendes virtual) |
| Sicherheitsscan (SAST) | CodeQL | Detektion von Speicherfehlern und Injektionsrisiken |
| Speicheranalyse | AddressSanitizer (ASan) | Dynamische Prüfung auf Laufzeitfehler |

Die Integration von CodeQL ist hierbei von höchster Priorität. CodeQL führt eine tiefgehende Datenflussanalyse durch, um Schwachstellen wie Pufferüberläufe oder Use-after-free-Fehler zu identifizieren, die bei KI-generiertem C++-Code aufgrund der manuellen Speicherverwaltung häufig auftreten können.20

### **Pull Request Governance und KI-Reviews**

Jeder Pull Request (PR), der KI-generierten Code enthält, sollte automatisch gelabelt werden (z.B. ai-generated), um die Aufmerksamkeit der menschlichen Reviewer zu schärfen.20 GitHub Copilot kann selbst in den Review-Prozess einbezogen werden, indem es PR-Zusammenfassungen erstellt oder potenzielle Risiken in Kommentaren erläutert.13 Dennoch darf die KI niemals die finale Merge-Entscheidung treffen.13 Ein "Human-in-the-Loop"-Ansatz ist zwingend erforderlich, wobei Senior-Entwickler besonders auf die Korrektheit der Geschäftslogik, Randfälle und die Einhaltung architektonischer Muster achten müssen.3

## **Methodisches Prompt Engineering für C++**

Effektives Prompting in der Systemprogrammierung unterscheidet sich signifikant von der Webentwicklung. Es erfordert eine präzise Definition der Systemumgebung und der Performance-Anforderungen.

### **Dekomposition und Spezifikation**

Komplexe C++-Aufgaben müssen in atomare Schritte zerlegt werden. Anstatt nach einem "effizienten Netzwerk-Layer" zu fragen, sollte der Prompt die Teilschritte definieren:

1. "Implementiere eine thread-sichere Queue unter Verwendung von std::mutex und std::condition\_variable."  
2. "Erstelle einen Socket-Handler, der asynchron Daten in diese Queue schreibt."  
3. "Verwende std::span für die pufferlose Verarbeitung der empfangenen Bytes".5

Durch die Bereitstellung von Acceptance-Kriterien und Testfällen im Prompt kann die KI ihre eigenen Ergebnisse vorab validieren.5 Ein Beispiel-Prompt könnte lauten: "Schreibe eine Funktion für den Token-Bucket-Algorithmus. Erstelle Unit-Tests, die verifizieren, dass bei einer Rate von ![][image1] Anfragen pro Sekunde die 11\. Anfrage innerhalb derselben Sekunde abgelehnt wird."

### **Fehlerkorrektur und Iteration**

Sollte die KI in die falsche Richtung steuern, ist es effizienter, den Kurs frühzeitig durch Follow-up-Prompts zu korrigieren, anstatt den gesamten Prozess neu zu starten.5 Die Verwendung von Checkpoints ermöglicht es, zu einem bekannten stabilen Zustand zurückzukehren, falls ein komplexes Refactoring durch einen KI-Agenten fehlschlägt.5

## **Governance, Sicherheit und langfristige Wartbarkeit**

Die Einführung von KI-Werkzeugen entbindet Organisationen nicht von der Notwendigkeit einer soliden Engineering-Kultur. Im Gegenteil, sie verstärkt die Auswirkungen bestehender Prozesse – gute Prozesse führen zu massiven Produktivitätsgewinnen, schwache Prozesse zu einer schnelleren Anhäufung von technischem Schuld und Sicherheitsrisiken.3

### **Die Rolle des Senior-Entwicklers in der KI-Ära**

Der erfahrene Entwickler wandelt sich vom Code-Schreiber zum "Decision Architect". Er ist verantwortlich für:

* Die Validierung der von der KI getroffenen Annahmen.5  
* Die Sicherstellung, dass die KI keine übermäßig komplexen oder unnötigen Abstraktionen einführt ("AI loves cleverness").20  
* Die Überwachung der Performance-Metriken, wie die Pull Request Cycle Time und die Deployment-Frequenz, um den tatsächlichen Impact der KI-Werkzeuge zu messen.3

### **Rechtliche und regulatorische Aspekte**

Bei der Nutzung von GitHub Copilot in Unternehmen müssen Richtlinien zur Datensicherheit und zum Schutz geistigen Eigentums beachtet werden. Dies umfasst das Filtern von Vorschlägen, die öffentlichem Code entsprechen könnten, sowie die Sicherstellung, dass keine sensiblen Daten oder Zugangsdaten in die Prompts gelangen.5 Die Verwendung von Enterprise-Policies ermöglicht es Organisationen, diese Einstellungen zentral zu steuern und die Einhaltung von Compliance-Vorgaben sicherzustellen.28

## **Fazit und Ausblick**

Die optimale Generierung und Konfiguration eines C++-Projekts für die KI-Ära erfordert eine ganzheitliche Betrachtung, die von der Dateistruktur im Repository bis hin zur automatisierten Verifizierung in der Cloud reicht. Durch die Kombination von tiefem semantischem Kontext in VS Code, präzisen Custom Instructions und einer rigorosen CI/CD-Pipeline können die spezifischen Herausforderungen von C++ – Komplexität, Fehleranfälligkeit und Performance-Druck – effektiv adressiert werden. KI-Werkzeuge wie GitHub Copilot fungieren dabei als mächtige Katalysatoren, welche die Entwicklungsgeschwindigkeit drastisch erhöhen, solange sie innerhalb klar definierter technologischer und organisatorischer Leitplanken operieren. Der Erfolg dieser Integration bemisst sich letztlich nicht an der Menge des generierten Codes, sondern an der Qualität, Sicherheit und Wartbarkeit des Gesamtsystems, für die weiterhin die menschliche Intelligenz die letzte Instanz bleibt.1

#### **Referenzen**

1. Mastering GitHub Copilot: Best Practices | by Wipro Tech Blogs | Mar, 2026 \- Medium, Zugriff am Mai 11, 2026, [https://wiprotechblogs.medium.com/mastering-github-copilot-best-practices-07c35f5cdfb7](https://wiprotechblogs.medium.com/mastering-github-copilot-best-practices-07c35f5cdfb7)  
2. Using C++ Development Tools with GitHub Copilot Chat \- Visual Studio Code, Zugriff am Mai 11, 2026, [https://code.visualstudio.com/docs/cpp/cpp-devtools](https://code.visualstudio.com/docs/cpp/cpp-devtools)  
3. GitHub Copilot Best Practices from High-Performing Teams | MetaCTO, Zugriff am Mai 11, 2026, [https://www.metacto.com/blogs/github-copilot-best-practices-from-high-performing-teams](https://www.metacto.com/blogs/github-copilot-best-practices-from-high-performing-teams)  
4. Improving GitHub Copilot's Context Awareness in Large Projects · community · Discussion \#188840, Zugriff am Mai 11, 2026, [https://github.com/orgs/community/discussions/188840](https://github.com/orgs/community/discussions/188840)  
5. Best practices for using AI in VS Code, Zugriff am Mai 11, 2026, [https://code.visualstudio.com/docs/copilot/best-practices](https://code.visualstudio.com/docs/copilot/best-practices)  
6. bkrabach/ai-code-project-template: Project template for ... \- GitHub, Zugriff am Mai 11, 2026, [https://github.com/bkrabach/ai-code-project-template](https://github.com/bkrabach/ai-code-project-template)  
7. GitHub Copilot in VS Code, Zugriff am Mai 11, 2026, [https://code.visualstudio.com/docs/copilot/overview](https://code.visualstudio.com/docs/copilot/overview)  
8. Microsoft brings C++ smarts to GitHub Copilot in Visual Studio Code \- InfoWorld, Zugriff am Mai 11, 2026, [https://www.infoworld.com/article/4136164/microsoft-brings-c-plus-plus-smarts-to-github-copilot-in-visual-studio-code.html](https://www.infoworld.com/article/4136164/microsoft-brings-c-plus-plus-smarts-to-github-copilot-in-visual-studio-code.html)  
9. C++ symbol context and CMake build configuration awareness for GitHub Copilot in VS Code \- Microsoft Developer Blogs, Zugriff am Mai 11, 2026, [https://devblogs.microsoft.com/cppblog/c-symbol-context-and-cmake-build-configuration-awareness-for-github-copilot-in-vs-code/](https://devblogs.microsoft.com/cppblog/c-symbol-context-and-cmake-build-configuration-awareness-for-github-copilot-in-vs-code/)  
10. awesome-copilot/instructions/cpp-language-service-tools.instructions.md at main \- GitHub, Zugriff am Mai 11, 2026, [https://github.com/github/awesome-copilot/blob/main/instructions/cpp-language-service-tools.instructions.md](https://github.com/github/awesome-copilot/blob/main/instructions/cpp-language-service-tools.instructions.md)  
11. Giving Copilot more C++ context using custom instructions in VS Code, Zugriff am Mai 11, 2026, [https://devblogs.microsoft.com/cppblog/giving-copilot-more-c-context-using-custom-instructions-in-vs-code/](https://devblogs.microsoft.com/cppblog/giving-copilot-more-c-context-using-custom-instructions-in-vs-code/)  
12. Use custom instructions in VS Code, Zugriff am Mai 11, 2026, [https://code.visualstudio.com/docs/copilot/customization/custom-instructions](https://code.visualstudio.com/docs/copilot/customization/custom-instructions)  
13. How to improve GitHub Copilot code review effectiveness and enforcement? · community · Discussion \#184163, Zugriff am Mai 11, 2026, [https://github.com/orgs/community/discussions/184163](https://github.com/orgs/community/discussions/184163)  
14. copilot-instructions\_qt-cpp.md \- GitHub Gist, Zugriff am Mai 11, 2026, [https://gist.github.com/smitmartijn/fc5c7025d88b4a4a70f068bd327ed6c1](https://gist.github.com/smitmartijn/fc5c7025d88b4a4a70f068bd327ed6c1)  
15. GitHub Copilot Instructions for loda-cpp, Zugriff am Mai 11, 2026, [https://github.com/loda-lang/loda-cpp/blob/main/.github/copilot-instructions.md](https://github.com/loda-lang/loda-cpp/blob/main/.github/copilot-instructions.md)  
16. C++ Coroutines: Complete Implementation Guide \- GitHub Gist, Zugriff am Mai 11, 2026, [https://gist.github.com/igaztanaga/906b47b7fe54603821ec1200d5f7176f](https://gist.github.com/igaztanaga/906b47b7fe54603821ec1200d5f7176f)  
17. C++20 Coroutines Demystified: Build a Generator From Scratch | by Sagar \- Medium, Zugriff am Mai 11, 2026, [https://medium.com/@sagar.necindia/cpp20-coroutines-demystified-part-1-generator-mental-model-67ace9265be3](https://medium.com/@sagar.necindia/cpp20-coroutines-demystified-part-1-generator-mental-model-67ace9265be3)  
18. Modernize your C++ project with GitHub Copilot modernization \- Microsoft Learn, Zugriff am Mai 11, 2026, [https://learn.microsoft.com/en-us/cpp/porting/copilot-app-modernization-cpp?view=msvc-170](https://learn.microsoft.com/en-us/cpp/porting/copilot-app-modernization-cpp?view=msvc-170)  
19. Project-Specific Build Optimizations with GitHub Copilot \- C++ Team Blog, Zugriff am Mai 11, 2026, [https://devblogs.microsoft.com/cppblog/project-specific-build-optimizations-with-github-copilot/](https://devblogs.microsoft.com/cppblog/project-specific-build-optimizations-with-github-copilot/)  
20. Best practices for using GitHub AI coding agents in production workflows? \#182197, Zugriff am Mai 11, 2026, [https://github.com/orgs/community/discussions/182197](https://github.com/orgs/community/discussions/182197)  
21. C/C++ Linter Action | clang-format & clang-tidy, Zugriff am Mai 11, 2026, [https://cpp-linter.github.io/cpp-linter-action/](https://cpp-linter.github.io/cpp-linter-action/)  
22. GitHub AI Code Review: 8 Copilot PR Automation Features, Zugriff am Mai 11, 2026, [https://www.augmentcode.com/tools/github-copilot-ai-code-review](https://www.augmentcode.com/tools/github-copilot-ai-code-review)  
23. Microsoft C++ Code Analysis with GitHub Actions, Zugriff am Mai 11, 2026, [https://devblogs.microsoft.com/cppblog/microsoft-cpp-code-analysis-with-github-actions/](https://devblogs.microsoft.com/cppblog/microsoft-cpp-code-analysis-with-github-actions/)  
24. Microsoft C++ Code Analysis Action \- GitHub Marketplace, Zugriff am Mai 11, 2026, [https://github.com/marketplace/actions/microsoft-c-code-analysis-action](https://github.com/marketplace/actions/microsoft-c-code-analysis-action)  
25. AI Code Review // VERY POWERFULL · Actions · GitHub Marketplace, Zugriff am Mai 11, 2026, [https://github.com/marketplace/actions/ai-code-review-very-powerfull](https://github.com/marketplace/actions/ai-code-review-very-powerfull)  
26. Set up GitHub Copilot in VS Code, Zugriff am Mai 11, 2026, [https://code.visualstudio.com/docs/copilot/setup](https://code.visualstudio.com/docs/copilot/setup)  
27. ai-prompt-engineering-safety-best-practices.instructions.md \- GitHub, Zugriff am Mai 11, 2026, [https://github.com/github/awesome-copilot/blob/main/instructions/ai-prompt-engineering-safety-best-practices.instructions.md](https://github.com/github/awesome-copilot/blob/main/instructions/ai-prompt-engineering-safety-best-practices.instructions.md)  
28. Managing GitHub Copilot & VS Code Settings Across Teams \- DEV Community, Zugriff am Mai 11, 2026, [https://dev.to/pwd9000/managing-github-copilot-vs-code-settings-across-teams-1phj](https://dev.to/pwd9000/managing-github-copilot-vs-code-settings-across-teams-1phj)  
29. Best practices for using GitHub Copilot, Zugriff am Mai 11, 2026, [https://docs.github.com/en/copilot/get-started/best-practices](https://docs.github.com/en/copilot/get-started/best-practices)

[image1]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADwAAAAYCAYAAACmwZ5SAAACIklEQVR4Xu2WT0gVURTGT2igkKgZRktdKEILJcRNCWIJCkagK81VkK4VFRUhCDcu2wTlpm20DYQWCoIorZ+tXOiiQNA26qKI+r45c2XmOPfxRnAGYn7ww/fOnXnON/evSEFBwf9EH/wB/0Y8gr/gH7gLR2CVuyEnuuCALYbUwjH4Dq7C9nhzMmvwN3wYqTHkS9Hgc/BGpC0LekT/71fRjpiPNwfUwy/wNbwFO+GeaCd5qYNbcB/eNW334IGn7bph4GE4BM8kOTBrfCGNkdo4/CZlnrcDHsNPsNq0dcNzWIJ3TFtWPJDkwAzJsB9Mnc98Cp+a+gVs4JCZtA3glWjbjKlniS+w6ygb2F2/YuoXvJHL8/cmfCH6g7Ph97zwBXZ1X2BbD+BE3xRdlbfDzxz/7NW3sMldWIbbcB0epnA5uLMyfIE5v/mcNljZwEnzl6vxgujq/CSs5Ykv8KBcIbCbv9Om7m7idpU3vsC+YL56QNL8JVza+SK8Ez8CRwSHPrewSm0I7qwMX+BW0UOTDeauXzT1svsvX4Rvs7dwQeuHoynkHlspvsBu/fkMayL1x6JrEv/GuA9/yuX9l58/SjzwkiT8QEZ4eww8F10EW8LvHG08de2InsICHomenuz5ObpRj4guWgw+Ad+LnlmzZAp+l/hznoiOyubwGo4u7iYb8Jlo2JLoETM1HOZc+rlSZx02DezVNtGp0iv5nhkKCgoKClLxDy4ckA8sQePXAAAAAElFTkSuQmCC>