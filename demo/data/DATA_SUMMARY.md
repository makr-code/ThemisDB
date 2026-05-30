# ThemisDB Demo Data Summary

## Verwaltungsrechtliche Demo-Daten
- **Dokumente (demo_articles):** 108
- **Vektoren (demo_embeddings):** 108
- **Graph-Knoten (demo_knowledge_graph):** 165
- **Graph-Kanten (demo_knowledge_graph):** 378

## Relationale Felder im Dokumentmodell
- case_id, applicant_id, authority_id
- procedure_type, legal_basis, status, region
- related_case_ids als Fall-Referenzen

## Graph-Node-Typen
- applicant: 40
- authority: 12
- case: 108
- court: 5

## Hinweis
Die Ingestion erfolgt im Demo-Setup weiterhin ueber Key/Blob-Payloads.
