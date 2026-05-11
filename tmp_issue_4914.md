## AI Update Blueprint

### Dateien, die konkret aktualisiert werden müssen
- mkdocs.yml
- mkdocs-nopdf.yml
- requirements-docs.txt
- docs/print-banner.html
- docs/print-cover.html
- docs/README-DOCUMENTATION.md
- docs/README.md
- docs/website/**
- docs/_generated/**
- docs/_Sidebar.md
- docs/_Footer.md

### Was genau zu ändern ist
- MkDocs-, Publishing- und Print/PDF-Pfade dokumentieren und auf einen konsistenten Build-Flow bringen.
- Die AI soll aus dem Ticket direkt erkennen, welche Dateien den Dokumentations-Deploy und die Website-Ausgabe steuern.
