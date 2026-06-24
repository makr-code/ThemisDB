# Thermodynamic and Entropy-Based Anomaly Detection Bibliography

## Scope

This document captures scientific references for entropy-/information-theory- and Boltzmann-inspired anomaly detection that are relevant to ThemisDB module-level monitoring (network, server, transaction, security, and RAG signal paths).

## Core References

1. Lakhina, A., Crovella, M., & Diot, C. (2004). *Diagnosing Network-Wide Traffic Anomalies*. ACM SIGCOMM.
2. Xu, K., Zhang, Z.-L., & Bhattacharyya, S. (2005). *Profiling Internet Backbone Traffic: Behavior Models and Applications*. ACM SIGCOMM IMC.
3. Wagner, A., & Plattner, B. (2005). *Entropy Based Worm and Anomaly Detection in Fast IP Networks*. IEEE WETICE.
4. Denning, D. E. (1987). *An Intrusion-Detection Model*. IEEE Transactions on Software Engineering, 13(2), 222–232.
5. Chandola, V., Banerjee, A., & Kumar, V. (2009). *Anomaly Detection: A Survey*. ACM Computing Surveys, 41(3), 15.
6. Cover, T. M., & Thomas, J. A. (2006). *Elements of Information Theory* (2nd ed.). Wiley.
7. Viegas, E., Santin, A. O., Bessani, A., Neves, N., & Oliveira, L. (2019). *A Big Data Approach to Detecting and Classifying Network Attacks*. Information Sciences, 501, 280–301.

## Boltzmann / Energy-Based Modeling References

1. Hinton, G. E. (2002). *Training Products of Experts by Minimizing Contrastive Divergence*. Neural Computation, 14(8), 1771–1800.
2. Hinton, G. E., Osindero, S., & Teh, Y.-W. (2006). *A Fast Learning Algorithm for Deep Belief Nets*. Neural Computation, 18(7), 1527–1554.
3. Fiore, U., Palmieri, F., Castiglione, A., & De Santis, A. (2013). *Network Anomaly Detection with the Restricted Boltzmann Machine*. Neurocomputing, 122, 13–23.

## Database / System Security Relevance

1. Kamra, A., Terzi, E., & Bertino, E. (2008). *Detecting Anomalous Access Patterns in Relational Databases*. VLDB Journal, 17, 1063–1077.
2. Sallam, A., Bertino, E., & Yegneswaran, V. (2015). *DBSAFE: An Anomaly Detection System to Protect Databases from Malicious Insiders*. IEEE Conference/Workshop literature on DB intrusion detection (reference family used for DB behavior profiling).
3. Bertino, E., Kamra, A., Terzi, E., & Vakali, A. (2005). *Intrusion Detection in RBAC-Administered Databases*. ACSAC/DB security literature.

## Mapping to ThemisDB Monitoring Hypothesis

- Entropy of protocol/endpoint/time-window distributions can characterize normal vs. abnormal behavior in `src/network` and `src/server` traffic paths.
- KL-divergence from baseline behavior can be used as module-local anomaly score in `src/security` and transaction telemetry.
- Boltzmann/energy-based models can represent a learned normal-state manifold for cross-module monitoring pipelines.

## Notes

- This is a research reference artifact (not a direct production claim).
- References should be validated against DOI/arXiv metadata when promoting individual citations into formal architecture or security policy documents.
