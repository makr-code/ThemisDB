## Update: Große Stub-/Simulation-Umsetzungen priorisiert (Core/Security/Tensor)

Neu verlinkte Issues:
- #4920 Core/Server Feature-Gates
- #4921 Security/Sharding SignedRequestVerifier
- #4922 Security/HSM PKCS#11
- #4923 Tensor TNSRTask Persistenz
- #4924 TensorRouter Template-Topologie
- #4925 HissReshaper pure-binary QTT
- #4926 HyperIndexBuilder FK-Propagation
- #4927 UTR semantische Encoder

### Priorisierung
1. P1 (Sicherheits- und Betriebsblocker)
- #4921
- #4922
- #4920

2. P2 (Persistenz-/Konsistenzkern Tensor)
- #4923
- #4924

3. P3 (Qualitäts-/Ranking-/Semantikverbesserungen Tensor)
- #4925
- #4926
- #4927

### Reihenfolgebegründung
- P1 zuerst wegen direkter Sicherheits- und Produktionsauswirkung.
- P2 danach, weil Persistenz-/Topologie-Korrektheit Grundlage für belastbare Tensor-Pipelines ist.
- P3 anschließend für Qualitäts- und Retrieval-Verbesserung auf stabiler Basis.
