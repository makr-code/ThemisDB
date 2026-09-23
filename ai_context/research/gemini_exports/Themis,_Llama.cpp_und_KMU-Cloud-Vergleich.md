# **Strategische Autonomie in der digitalen Verwaltung: Die Integration von llama.cpp in ThemisDB als Fundament für souveräne KI-Infrastrukturen im deutschen Mittelstand und im öffentlichen Sektor**

Die fortschreitende Integration künstlicher Intelligenz in die Kernprozesse der deutschen Wirtschaft und Verwaltung markiert eine Zäsur in der digitalen Transformation. In einer Ära, in der Sprachmodelle (Large Language Models, LLMs) zunehmend als Basistechnologie für Wissensmanagement, Prozessautomatisierung und Bürgerkommunikation fungieren, rückt die Frage nach der zugrunde liegenden Infrastruktur in das Zentrum der strategischen Debatte. Während globale Hyperscaler wie Microsoft Azure, Amazon Web Services (AWS) und Google Cloud Platform (GCP) leistungsstarke, aber proprietäre Ökosysteme anbieten, gewinnt der Ansatz der lokalen Inferenz durch spezialisierte Architekturen wie die Themis-Plattform an Bedeutung.1 Die technologische Basis hierfür bildet die Integration von llama.cpp, einer hocheffizienten C++-Bibliothek, die LLM-Inferenz auf heterogener Hardware ermöglicht und damit die technologische Brücke zwischen höchster Performance und strikter Datensouveränität schlägt.4

## **Technologische Architektur und die Rolle von llama.cpp**

Die Entscheidung der Themis-Plattform (insbesondere ThemisDB v1.3.0), auf llama.cpp als primäre Inferenz-Engine zu setzen, basiert auf einer tiefgreifenden Analyse von Ressourceneffizienz und Systemintegration. Im Gegensatz zu weit verbreiteten Python-basierten Frameworks, die oft eine komplexe Abhängigkeitsstruktur aufweisen, zeichnet sich llama.cpp durch eine native Implementierung in C/C++ aus.3 Diese Wahl ermöglicht es, die Inferenz-Engine direkt in den Kern der Datenbank-Engine einzubetten, was durch das Kompilierungs-Flag \-DTHEMIS\_ENABLE\_LLM=ON realisiert wird.3

### **Native Integration versus Container-Abstraktion**

Ein wesentlicher Vorteil dieser Architektur liegt in der Reduktion des Overheads. Während moderne Inferenz-Server wie vLLM oder Ollama oft als eigenständige Prozesse oder in Docker-Containern betrieben werden, agiert llama.cpp innerhalb von ThemisDB als integrierte Bibliothek.3 Dies eliminiert die Notwendigkeit für Interprozesskommunikation (IPC) oder Netzwerk-Hops über gRPC-Schnittstellen, was insbesondere für kleine und mittlere Unternehmen (KMU) mit begrenzten Rechenkapazitäten von entscheidender Bedeutung ist. Die Speicherverwaltung profitiert hierbei massiv: Die Themis-Implementierung benötigt einen VRAM-Overhead von lediglich 150 bis 300 MB, während Python-basierte Alternativen oft 500 bis 800 MB allein für die Laufzeitumgebung beanspruchen.3

Die Hardware-Abstraktion erfolgt über einen spezialisierten Acceleration Plugin Loader, der eine breite Palette von Backends unterstützt.3 Hierbei zeigt sich die Vielseitigkeit von llama.cpp, die von NVIDIA CUDA über AMDs HIP (ROCm) bis hin zu Vulkan und Apples Metal-Framework reicht.4 Für die Verwaltung der Bundesländer, die oft auf heterogene Hardwarebestände in ihren Rechenzentren (z. B. ITZBund oder Landesrechenzentren) zurückgreifen müssen, bietet diese Flexibilität einen Investitionsschutz, da keine strikte Bindung an einen spezifischen Chiphersteller besteht.3

### **Das GGUF-Format und effiziente Speicherverwaltung**

Das Herzstück der Inferenz-Effizienz ist das GGUF-Modellformat (GGML Universal File), das als Nachfolger älterer binärer Formate konzipiert wurde, um Erweiterbarkeit und schnelle Ladezeiten zu garantieren.4 In ThemisDB wird dieses Format genutzt, um Modelle mittels Memory-Mapping (mmap) zu laden.3 Dieser Mechanismus erlaubt es dem Betriebssystem, Modellteile nur dann physisch in den RAM zu laden, wenn sie tatsächlich für die Berechnung benötigt werden. In Kombination mit mlock, das kritische Modellseiten im Speicher fixiert, wird eine konstante Inferenzgeschwindigkeit sichergestellt, ohne das System durch unnötiges Swapping zu verlangsamen.3

Mathematisch lässt sich der Vorteil der in llama.cpp integrierten Quantisierung verdeutlichen. Durch die Reduktion der Präzision der Modellgewichte von 16-Bit-Fließkommazahlen (FP16) auf beispielsweise 4-Bit-Ganzzahlen (Q4\_K\_M) sinkt der Speicherbedarf $S$ signifikant bei minimalem Genauigkeitsverlust:

$$S\_{quantisiert} \\approx P \\times \\frac{b}{8} \\text{ (in Gigabyte)}$$  
Wobei $P$ die Anzahl der Parameter in Milliarden und $b$ die Bit-Tiefe darstellt. Ein Llama-3-8B Modell reduziert seinen Bedarf so von etwa 16 GB auf ca. 4,5 GB, was den Betrieb leistungsfähiger KI-Assistenten auf handelsüblichen KMU-Servern oder sogar modernen Workstations ermöglicht.6

| Komponente | llama.cpp (ThemisDB) | vLLM | Hyperscaler (Cloud API) |
| :---- | :---- | :---- | :---- |
| **Implementierung** | C++ Native | Python / CUDA Kernels | Proprietary |
| **Speicherbedarf (Base)** | \~250 MB RAM / \~150 MB VRAM | 2-4 GB RAM / \~500 MB VRAM | Keine lokale Hardware |
| **Startup-Zeit** | \< 1 Sekunde | 5-10 Sekunden | Millisekunden (API ready) |
| **Datenkontrolle** | Vollständig lokal | Lokal / Container | Externer Anbieter |
| **Hardware-Fokus** | CPU, GPU, Apple Silicon | High-End NVIDIA GPUs | Rechenzentren des Anbieters |

## **Leistungsvergleich: On-Premise-Inferenz gegen Cloud-Hyperscaler**

Der direkte Vergleich zwischen der lokalen Inferenz in ThemisDB und den Cloud-Angeboten von Azure OpenAI, Google Vertex AI oder AWS Bedrock offenbart fundamentale Unterschiede in der Performance-Charakteristik, die insbesondere für zeitkritische Anwendungen in der Verwaltung und im Mittelstand relevant sind.3

### **Latenz- und Durchsatzmetriken**

In cloudbasierten Systemen ist die Latenz untrennbar mit der geografischen Distanz zum Rechenzentrum und der Komplexität der Netzwerk-Infrastruktur verbunden.10 Die Themis-Plattform eliminiert den Netzwerk-Overhead durch das On-Premise-Deployment vollständig.3

Die Performance-Vorteile manifestieren sich in zwei Schlüsselbereichen: der Inferenz-Latenz für den ersten Token und dem Gesamtdurchsatz bei parallelen Anfragen. Laut internen Benchmarks von ThemisDB v1.3.0 liegt die Ziel-Latenz für den ersten Token bei 28 ms (p50), während Azure OpenAI unter vergleichbaren Bedingungen etwa 120 ms benötigt.3 Dieser Vorsprung von Faktor 4 bis 5 ist entscheidend für Anwendungen wie interaktive Chatbots oder Echtzeit-Schreibassistenten, bei denen eine Verzögerung unmittelbar die Nutzererfahrung beeinträchtigt.3

### **Der Vorteil des Semantic Caching**

Ein oft übersehener Aspekt der Themis-Architektur ist das integrierte Semantic Caching. Da ThemisDB eine Vektordatenbank nativ mit der LLM-Inferenz verknüpft, kann das System semantisch ähnliche Anfragen identifizieren, bevor sie die teure Inferenz-Phase erreichen.3 Während Hyperscaler oft nur einfache, nutzerspezifische Caches anbieten (mit Trefferraten von ca. 20-30%), erreicht ThemisDB durch die Kombination von SemanticCache und EmbeddingCache effektive Hit-Rates von 70-90%.3 In der Praxis bedeutet dies, dass wiederkehrende Bürgeranfragen oder standardisierte KMU-Prozesse nicht jedes Mal neu berechnet werden müssen, was die durchschnittliche Antwortzeit von 150 ms auf 28 ms senkt – ein Geschwindigkeitsvorteil von 5,4x.3

| Anbieter / Plattform | First Token Latency (p50) | Durchsatz (Batch) | RAG-Latenz (Gesamt) |
| :---- | :---- | :---- | :---- |
| **ThemisDB (On-Prem)** | 28 ms | 128 req/s | 50–70 ms |
| **Azure OpenAI** | 120 ms | 65 req/s | 200–300 ms |
| **Google Vertex AI** | 110 ms | 70 req/s | 250 ms (geschätzt) |
| **AWS Bedrock** | 130 ms | 60 req/s | 300 ms (geschätzt) |

Die drastisch reduzierte RAG-Latenz (Retrieval-Augmented Generation) in ThemisDB ist auf den sogenannten "Zero-Hop RAG"-Ansatz zurückzuführen.3 Hierbei werden Suchergebnisse aus der integrierten Graph- und Vektordatenbank ohne Serialisierung oder Netzwerkübertragung direkt in den Kontext des LLM übergeben. Dies vermeidet die Latenzspitzen, die bei cloudbasierten RAG-Systemen entstehen, wenn Daten zwischen Vektorsuche (z. B. Pinecone oder Azure AI Search) und dem Modell-Endpunkt fließen.3

## **Wirtschaftlichkeitsanalyse und Total Cost of Ownership (TCO)**

Für KMU und die öffentliche Verwaltung ist die wirtschaftliche Nachhaltigkeit von KI-Investitionen ein primärer Entscheidungsfaktor. Der Übergang von einem rein operativen Ausgabenmodell (OPEX) bei Hyperscalern zu einem kapitalbasierten Modell (CAPEX) bei On-Premise-Lösungen erfordert eine differenzierte Betrachtung des Return on Investment (ROI).10

### **Vergleich der Betriebskosten**

Die Kostenstruktur von Cloud-LLMs basiert typischerweise auf der Anzahl der verarbeiteten Token. Bei einem hohen Aufkommen, wie es in der automatisierten Dokumentenverarbeitung einer Landesverwaltung oder im Kundensupport eines mittelständischen Unternehmens auftritt, skalieren diese Kosten linear und können schnell fünf- bis sechsstellige Beträge pro Monat erreichen.3

ThemisDB bietet hier einen radikalen Kostenvorteil. Basierend auf einem Volumen von 1 Million Token pro Tag ergeben sich für ThemisDB monatliche Betriebskosten von etwa 1.200 USD (inklusive Hardware-Amortisation, Energie und Wartung).3 Im Vergleich dazu belaufen sich die Kosten für Azure OpenAI (GPT-3.5 Turbo) auf ca. 60.000 USD und für Google Vertex auf ca. 45.000 USD.3 Über einen Zeitraum von drei Jahren betrachtet, führt die Nutzung von ThemisDB zu einer Kostenersparnis von bis zu 98% gegenüber führenden Hyperscalern.3

### **Break-Even-Analyse und Amortisation**

Besonders beeindruckend ist die kurze Zeitspanne bis zum Break-Even-Punkt. Die initiale Investition für einen leistungsfähigen Server inklusive einer NVIDIA A100 GPU (ca. 10.000 USD) amortisiert sich im Vergleich zur Nutzung von Azure OpenAI bereits nach etwa 6 Tagen Dauerbetrieb.3 Für KMU ist zudem die steuerliche Komponente relevant: On-Premise-Hardware kann abgeschrieben werden, was die effektiven Kosten weiter senkt, während Cloud-Gebühren zwar abzugsfähig, aber permanent anfallende Kostenstellen ohne Vermögensbildung darstellen.11

Für die Verwaltung der Bundesländer bietet das lokale Modell zudem Budget-Sicherheit. Während Cloud-Anbieter ihre Preisstrukturen oder API-Limits kurzfristig anpassen können, bietet die eigene Infrastruktur eine kalkulierbare Basis für langfristige Projekte.6 Dies ist insbesondere bei Projekten mit einer Laufzeit von 2026 bis 2030, wie sie im Rahmen der deutsch-französischen KI-Partnerschaft geplant sind, von strategischem Wert.13

## **Digitale Souveränität und die Strategie der Bundesländer**

Der Begriff der digitalen Souveränität hat sich von einem politischen Schlagwort zu einer technischen Anforderung entwickelt. Deutschland verfolgt das Ziel, zentrale digitale Infrastrukturen und Schlüsseltechnologien eigenständig und nach eigenen Regeln zu gestalten.1

### **Die Rolle der Deutschen Verwaltungscloud (DVC)**

Mit dem Start der Deutschen Verwaltungscloud (DVC) im März 2025 wurde ein Meilenstein erreicht.2 Die DVC fungiert als Multi-Cloud-Ökosystem, das föderale IT-Dienstleister vernetzt und den Austausch souveräner Cloud-Services ermöglicht. ThemisDB fügt sich hier nahtlos ein, indem es eine Architektur bietet, die sowohl lokal (On-Premise) in den Rechenzentren der Länder als auch als Teil der DVC-Infrastruktur betrieben werden kann.2

Ein zentraler Aspekt der DVC ist die Vermeidung von Lock-in-Effekten.2 Durch die Nutzung von Open-Source-Komponenten wie llama.cpp und standardisierten Schnittstellen (OpenAPI-Kompatibilität) stellt Themis sicher, dass Verwaltungen jederzeit zwischen verschiedenen Modellen oder Infrastrukturanbietern wechseln können, ohne ihre gesamte Applikationslogik neu entwickeln zu müssen.1

### **Der "Germany Stack" und Open Source in der Verwaltung**

Die Bundesregierung setzt verstärkt auf den sogenannten "Germany Stack" – ein Set aus quelloffenen Technologien, die als digitales Betriebssystem für die Verwaltung fungieren.14 Plattformen wie openCode.de dienen dabei als zentrales Repository, um bewährte Lösungen zwischen Behörden zu teilen und gemeinsam weiterzuentwickeln.16 Die Integration von KI-Modellen in diesen Stack erfordert Technologien, die transparent und prüfbar sind.

Die Studie des Kompetenzzentrums Öffentliche IT (Fraunhofer FOKUS) unterstreicht, dass Inhouse-Entwicklungen die Abhängigkeit von globalen Konzernen massiv reduzieren können.1 Die Empfehlung lautet klar auf den Ausbau gemeinsamer LLM-Infrastrukturen und die Förderung von Open-Source-Ansätzen.1 ThemisDB adressiert diese Anforderungen direkt durch die Unterstützung von LoRA-Adaptern, die es ermöglichen, universelle Basismodelle mit spezifischem Wissen der deutschen Verwaltung (z. B. juristische Datenbanken oder Verwaltungsvorschriften) zu verfeinern, ohne die Hoheit über diese sensiblen Trainingsdaten aufgeben zu müssen.3

## **Administrative Use Cases und föderale Anforderungen**

Die deutsche Verwaltungsstruktur ist durch den Föderalismus geprägt, was zu einer hohen Komplexität bei der Implementierung von IT-Systemen führt. Jedes Bundesland hat eigene Gesetze, Verordnungen und Prozesse, die in KI-gestützten Systemen berücksichtigt werden müssen.3

### **Unterstützung von Baugenehmigungsverfahren**

Ein prägnantes Beispiel ist das Baugenehmigungsverfahren. Die Bauordnungen variieren zwischen Bayern, Nordrhein-Westfalen und Berlin erheblich.3 Die Themis-Plattform löst diese Herausforderung durch eine Sharding-Architektur, bei der für jedes Bundesland dedizierte Shards betrieben werden, die mit spezialisierten LoRA-Adaptern ausgestattet sind.3

| Bundesland / Shard | Spezialisierter LoRA-Adapter | Fokusbereich |
| :---- | :---- | :---- |
| **Bayerische Verwaltung** | baurecht\_bayern\_v1 | Alpinbau, Denkmalschutz |
| **NRW (Landesportal)** | baurecht\_nrw\_v1 | Hochwasserschutz, urbane Verdichtung |
| **Berlin (Senatsverwaltung)** | baurecht\_berlin\_v1 | Monumentalschutz, Nachkriegsbauten |

Diese spezialisierten Adapter ermöglichen es dem System, Bürgeranfragen präzise unter Angabe der korrekten Paragraphen (z. B. BauO NRW § 6\) zu beantworten.3 Die KI fungiert hier als Entscheidungsunterstützung für Sachbearbeiter, indem sie "kritische Pfade" in Anträgen identifiziert und die Bearbeitungsdauer durch automatisierte Vorprüfungen verkürzt.12

### **Polizeiliche Ermittlungsunterstützung und Justiz**

In der Kriminalitätsbekämpfung ermöglicht die Integration von llama.cpp und Vektorsuche die Analyse komplexer Fallakten in einer gesicherten On-Premise-Umgebung. Shards in verschiedenen Bundesländern können sich auf unterschiedliche Kriminalitätsphänomene spezialisieren (z. B. Cybercrime in NRW oder Grenzkriminalität in Bayern), während anonymisierte Erkenntnisse über gesicherte Schnittstellen geteilt werden.3

Im juristischen Bereich unterstützen spezialisierte Adapter Richter und Staatsanwälte bei der Recherche in Präzedenzfällen. Hierbei ist die Traceability (Nachvollziehbarkeit) von entscheidender Bedeutung.3 Die Themis-Architektur stellt sicher, dass jede KI-generierte Antwort direkt auf die zugrunde liegende Datenquelle in der Datenbank verweist, was das Risiko von Halluzinationen minimiert und die Anforderungen des AI Act an Erklärbarkeit erfüllt.12

## **Sicherheit und Compliance nach BSI IT-Grundschutz**

Für KMU und Behörden ist die Einhaltung regulatorischer Vorgaben wie der DSGVO und des BSI IT-Grundschutzes nicht optional. Cloud-Lösungen stoßen hier oft an Grenzen, da die physische Kontrolle über die Daten fehlt und Telemetriedaten oft unkontrolliert abfließen.15

### **Kontrolle über Datenströme und Telemetrie**

Die Plattform Delos Cloud versucht zwar, Microsoft-Technologien in einer souveränen deutschen Umgebung bereitzustellen, doch verbleibt eine Abhängigkeit von den Update-Zyklen und der grundlegenden Architektur des Herstellers.15 Ein On-Premise-System mit llama.cpp bietet hingegen die vollständige Autarkie. Funktionale Sicherheit ist im Krisenfall auch ohne externe Updates gewährleistet.15

Der IT-Grundschutz bietet klare Module für die Absicherung von Serverinfrastrukturen (SYS.1.1) und den IT-Betrieb (OPS.1.1). Durch den Betrieb von ThemisDB können Organisationen bewährte Methoden für das Identitäts- und Rechtemanagement (IAM) und privilegierte Konten (PAM) direkt auf die KI-Infrastruktur anwenden.19 Da keine Daten über das Internet an externe APIs übertragen werden, entfallen komplexe Prüfungen der Verschlüsselung während der Übertragung (TLS-Absicherung von Endpunkten) und die Sorge vor Datenabflüssen bei Drittanbietern.10

### **Ethische KI und Bias-Management**

Die Themis-Plattform (insbesondere das THEMIS 5.0 Projekt) legt einen starken Fokus auf ethische KI. Ein spezielles Modul zur Bewertung von Fairness berechnet Metriken für systemische, algorithmische und menschliche Verzerrungen (Bias).21 Da die Inferenz lokal erfolgt, können diese Überprüfungen kontinuierlich und ohne Kosten für externe API-Aufrufe durchgeführt werden. Dies ermöglicht es Behörden, die Konformität ihrer KI-Systeme mit gesellschaftlichen Werten und rechtlichen Normen der EU permanent zu überwachen.21

## **Die Brücke zum Anwender: Deklarative KI mit AQL**

Ein kritischer Erfolgsfaktor für KMU ist die einfache Bedienbarkeit der Technologie. Viele Unternehmen verfügen nicht über spezialisierte Data Scientists oder Python-Entwickler, um komplexe KI-Pipelines aufzubauen.3

### **Abstraktion technischer Komplexität**

ThemisDB löst dieses Problem durch eine Erweiterung der Abfragesprache AQL (ArangoDB Query Language). LLM-Operationen werden zu erstklassigen Bürgern der Datenbankabfrage.3 Anstatt wie in der Cloud-Welt mehrere APIs (Embedding-API, Vector-DB-API, LLM-Inferenz-API) mit komplexem "Glue Code" zu verbinden, ermöglicht ThemisDB deklarative Abfragen.

Ein typisches Szenario für ein KMU könnte die automatisierte Zusammenfassung von Kundenfeedback sein. Mit AQL reduziert sich dies auf einen einzigen Befehl:

Code-Snippet

FOR feedback IN kunden\_rezensionen  
  LET zusammenfassung \= LLM INFER feedback.text  
    USING MODEL 'mistral-7b'  
    WITH LORA 'summarization'  
  RETURN { kunde: feedback.name, summary: zusammenfassung }

Diese Abstraktion sorgt dafür, dass die technische Komplexität der Inferenz-Engine llama.cpp (wie das Management von GPU-Layern oder Thread-Konfigurationen) vor dem Anwender verborgen bleibt.3 Gleichzeitig optimiert das System im Hintergrund automatisch die Performance durch Prefix-Caching und parallele Ausführung über den AsyncInferenceEngine.3

### **Integration in bestehende IT-Landschaften**

Für KMU ist die Integrationsfähigkeit entscheidend. Die lokale Inferenz über llama.cpp in ThemisDB bietet standardisierte Schnittstellen wie Stdio oder Server-Sent Events (SSE), die eine nahtlose Einbindung in Webanwendungen oder ERP-Systeme ermöglichen.5 Dies erlaubt es Unternehmen, KI-Funktionen "offline" und lokal zu nutzen, ohne ihre bestehende Sicherheitsarchitektur durch externe Cloud-Verbindungen aufbrechen zu müssen.5

## **Strategische Schlussfolgerungen und Ausblick**

Die Analyse der Integration von llama.cpp in die Themis-Plattform verdeutlicht eine fundamentale Verschiebung im KI-Markt. Für den deutschen Mittelstand und die öffentliche Verwaltung stellt die lokale Inferenz nicht nur eine technische Alternative, sondern eine strategische Notwendigkeit dar.1

### **Zusammenfassung der Vorteile**

1. **Performance**: Lokale Inferenz eliminiert Netzwerk-Latenzen und bietet durch Semantic Caching eine Geschwindigkeit, die Cloud-Lösungen um den Faktor 5 bis 7 übertrifft.3  
2. **Kosteneffizienz**: Bei produktiver Nutzung amortisiert sich On-Premise-Hardware in Rekordzeit. Die TCO-Ersparnis von bis zu 98% ermöglicht es auch KMU, KI-Projekte wirtschaftlich erfolgreich umzusetzen.3  
3. **Souveränität**: Durch die Nutzung von Open-Source-Technologien und lokalen Deployment-Modellen gewinnen Behörden und Unternehmen die vollständige Kontrolle über ihre Daten und Prozesse zurück.1  
4. **Flexibilität**: Die Unterstützung heterogener Hardware und spezialisierter LoRA-Adapter erlaubt eine präzise Anpassung an föderale Anforderungen und spezifische Geschäftsanforderungen, ohne in die Abhängigkeit eines einzelnen Anbieters zu geraten.3

### **Handlungsempfehlungen**

Für Entscheidungsträger im öffentlichen Sektor und in KMU empfiehlt sich ein stufenweises Vorgehen. Initial sollten Pilotprojekte auf Basis von Open-Source-Modellen und lokaler Inferenz gestartet werden, um interne Kompetenzen ("Gestaltungsfähigkeit") aufzubauen.1 Die Einbettung dieser Projekte in souveräne Ökosysteme wie die Deutsche Verwaltungscloud oder den "Germany Stack" sichert die langfristige Interoperabilität und Teilhabe an gemeinschaftlichen Entwicklungen.2

In der Ära der generativen KI wird die Fähigkeit, technologische Lösungen eigenständig zu betreiben und zu kontrollieren, zu einem entscheidenden Wettbewerbsvorteil und einer Grundvoraussetzung für das Vertrauen der Bürger in den digitalen Staat.16 Die Kombination aus der Effizienz von llama.cpp und der strukturellen Stärke von ThemisDB bietet hierfür das technologische Fundament. Während Hyperscaler für schnelle Experimente und unkritische Anwendungen weiterhin relevant bleiben, gehört die Zukunft der wertschöpfenden und hoheitlichen KI-Anwendungen der souveränen On-Premise-Inferenz.

#### **Referenzen**

1. Digitale Souveränität und große Sprachmodelle in der ... \- BMDS, Zugriff am Dezember 28, 2025, [https://bmds.bund.de/service/publikationen/digitale-souveraenitaet-und-grosse-sprachmodelle-in-der-bundesverwaltung](https://bmds.bund.de/service/publikationen/digitale-souveraenitaet-und-grosse-sprachmodelle-in-der-bundesverwaltung)  
2. Press \- Germany launches government cloud \- BMI, Zugriff am Dezember 28, 2025, [https://www.bmi.bund.de/SharedDocs/pressemitteilungen/EN/2025/03/dvc.html](https://www.bmi.bund.de/SharedDocs/pressemitteilungen/EN/2025/03/dvc.html)  
3. LLM\_LOADER\_GUIDE.md  
4. Llama.cpp Meets Instinct: A New Era of Open-Source AI Acceleration \- ROCm™ Blogs, Zugriff am Dezember 28, 2025, [https://rocm.blogs.amd.com/ecosystems-and-partners/llama-cpp/README.html](https://rocm.blogs.amd.com/ecosystems-and-partners/llama-cpp/README.html)  
5. Die 5 besten LLM-Tools zur lokalen Ausführung von Modellen \- Apidog, Zugriff am Dezember 28, 2025, [https://apidog.com/de/blog/top-llm-local-tools-5/](https://apidog.com/de/blog/top-llm-local-tools-5/)  
6. Was ist Llama.cpp? Der umfassende Ratgeber \- Biteno GmbH, Zugriff am Dezember 28, 2025, [https://www.biteno.com/was-ist-llama-cpp/](https://www.biteno.com/was-ist-llama-cpp/)  
7. Lokale KI: Einblicke in Ollama und Llama.cpp \- Infralovers, Zugriff am Dezember 28, 2025, [https://www.infralovers.com/de/blog/2024-07-09-empowering-local-ai/](https://www.infralovers.com/de/blog/2024-07-09-empowering-local-ai/)  
8. AI and the cloud: key factors for future-proof administration \- Smart Country Convention, Zugriff am Dezember 28, 2025, [https://www.smartcountry.berlin/en/newsblog/ai-and-the-cloud-key-factors-for-future-proof-administration.html](https://www.smartcountry.berlin/en/newsblog/ai-and-the-cloud-key-factors-for-future-proof-administration.html)  
9. Lokale LLM \- ChatGPT ohne Cloud 2025 \- Mittelstand-Digital Zentrum Hamburg, Zugriff am Dezember 28, 2025, [https://digitalzentrum-hamburg.de/leitfaden/lokale-llm-ohne-cloud/](https://digitalzentrum-hamburg.de/leitfaden/lokale-llm-ohne-cloud/)  
10. On Premise vs Cloud Based LLM: Which Is Right for Your Industry?, Zugriff am Dezember 28, 2025, [https://www.signitysolutions.com/blog/on-premise-vs-cloud-based-llm](https://www.signitysolutions.com/blog/on-premise-vs-cloud-based-llm)  
11. How to Choose the Best Deployment Model for Enterprise AI: Cloud vs On-Prem, Zugriff am Dezember 28, 2025, [https://www.allganize.ai/en/blog/enterprise-guide-choosing-between-on-premise-and-cloud-llm-and-agentic-ai-deployment-models](https://www.allganize.ai/en/blog/enterprise-guide-choosing-between-on-premise-and-cloud-llm-and-agentic-ai-deployment-models)  
12. The Complete Guide to Using AI in the Government Industry in Germany in 2025, Zugriff am Dezember 28, 2025, [https://www.nucamp.co/blog/coding-bootcamp-germany-deu-government-the-complete-guide-to-using-ai-in-the-government-industry-in-germany-in-2025](https://www.nucamp.co/blog/coding-bootcamp-germany-deu-government-the-complete-guide-to-using-ai-in-the-government-industry-in-germany-in-2025)  
13. France and Germany Join Forces with Mistral AI and SAP SE to Launch a Sovereign AI for Public Administration \- BMDS, Zugriff am Dezember 28, 2025, [https://bmds.bund.de/aktuelles/pressemitteilungen/detail/france-and-germany-join-forces-with-mistral-ai-and-sap-se-to-launch-a-sovereign-ai-for-public-administration](https://bmds.bund.de/aktuelles/pressemitteilungen/detail/france-and-germany-join-forces-with-mistral-ai-and-sap-se-to-launch-a-sovereign-ai-for-public-administration)  
14. Digital strategy in the 2025 coalition agreement \- mailbox, Zugriff am Dezember 28, 2025, [https://mailbox.org/en/news/digital-strategy-in-the-2025-coalition-agreement/](https://mailbox.org/en/news/digital-strategy-in-the-2025-coalition-agreement/)  
15. Delos Cloud – Die souveräne Cloud für den Öffentlichen Dienst, Zugriff am Dezember 28, 2025, [https://www.deloscloud.de/index.html](https://www.deloscloud.de/index.html)  
16. openCode.de \- Open-Source-Plattform für die öffentliche Verwaltung, Zugriff am Dezember 28, 2025, [https://opencode.de/](https://opencode.de/)  
17. Code teilen und entwickeln \- openCode.de, Zugriff am Dezember 28, 2025, [https://opencode.de/code-teilen-und-entwickeln](https://opencode.de/code-teilen-und-entwickeln)  
18. Infrastruktur und Standards für Generative KI in der Öffentlichen Verwaltung \- IT-Planungsrat, Zugriff am Dezember 28, 2025, [https://www.it-planungsrat.de/fileadmin/it-planungsrat/der-it-planungsrat/schwerpunktthemen/SPTDatennutzung\_241118\_Zwischenbericht\_KT\_KI\_MT2\_0.9.pdf](https://www.it-planungsrat.de/fileadmin/it-planungsrat/der-it-planungsrat/schwerpunktthemen/SPTDatennutzung_241118_Zwischenbericht_KT_KI_MT2_0.9.pdf)  
19. BSI IT-Grundschutz: Einstieg, Bausteine, Best Practices | IPG \- Experten für IAM, Zugriff am Dezember 28, 2025, [https://www.ipg-group.com/blog/expertenberichte/bsi-it-grundschutz](https://www.ipg-group.com/blog/expertenberichte/bsi-it-grundschutz)  
20. LLM Deployment: A Simple Guide to Cloud vs. On-Premises \- Maruti Techlabs, Zugriff am Dezember 28, 2025, [https://marutitech.com/llm-deployment-guide-cloud-vs-on-premises/](https://marutitech.com/llm-deployment-guide-cloud-vs-on-premises/)  
21. Transforming Ethical Considerations in AI \- THEMIS 5.0, Zugriff am Dezember 28, 2025, [https://www.themis-trust.eu/post/themis-5-0-transforming-ethical-considerations-in-ai](https://www.themis-trust.eu/post/themis-5-0-transforming-ethical-considerations-in-ai)  
22. opencode-ai/opencode: A powerful AI coding agent. Built for the terminal. \- GitHub, Zugriff am Dezember 28, 2025, [https://github.com/opencode-ai/opencode](https://github.com/opencode-ai/opencode)