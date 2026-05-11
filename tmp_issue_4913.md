## AI Update Blueprint

### Dateien, die konkret aktualisiert werden müssen
- build_*.txt
- test_*.txt
- tmp_*.md
- sec_block.txt
- scout_cves_*.sarif

### Was genau zu ändern ist
- Nicht-kanonische Root-Artefakte inventarisieren, klassifizieren und von den eigentlichen Leitdokumenten trennen.
- Die AI soll erkennen, welche Dateien nur Build-/Test-/Scan-Ausgaben sind und nicht als redaktionelle Quelle behandelt werden dürfen.
