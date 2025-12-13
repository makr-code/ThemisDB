# Function Dependency Graph

This document provides visual diagrams showing the function call relationships and dependencies for the stub replacement implementations.

## Overall Architecture

```mermaid
graph TB
    subgraph "External Systems"
        NTP[NTP Servers]
        OpenSSL[OpenSSL Library]
        RocksDB[RocksDB Storage]
    end
    
    subgraph "ThemisDB Core - Modified Components"
        PS[PluginSecurityVerifier]
        MD[ManifestDatabase]
        TT[TrueTime]
        PM[ProcessMining]
    end
    
    subgraph "Integration Layer"
        PL[Plugin Loader]
        HR[Hot Reload Handler]
        DT[Distributed Transaction]
        QE[Query Engine]
    end
    
    subgraph "Storage Layer"
        SI[Secondary Index]
        GI[Graph Index]
        PG[Process Graph]
    end
    
    %% External connections
    TT -->|NTP Protocol| NTP
    PS -->|X.509 Verify| OpenSSL
    MD -->|Store/Retrieve| RocksDB
    
    %% Component relationships
    MD -->|Uses| PS
    HR -->|Uses| MD
    PL -->|Uses| PS
    DT -->|Uses| TT
    PM -->|Uses| GI
    PM -->|Uses| PG
    QE -->|Uses| PM
    
    style PS fill:#90EE90
    style MD fill:#90EE90
    style TT fill:#90EE90
    style PM fill:#90EE90
```

## Plugin Security Verifier - Function Graph

```mermaid
graph LR
    subgraph "Public API"
        VP[verifyPlugin]
        VS[verifySignature]
        CFH[calculateFileHash]
        LM[loadMetadata]
        GTL[getTrustLevel]
    end
    
    subgraph "Helper Functions"
        DHS[decodeHexString]
        IBL[isBlacklisted]
        IWL[isWhitelisted]
    end
    
    subgraph "OpenSSL Functions"
        X509[X509_*]
        EVP[EVP_DigestVerify]
        PEM[PEM_read_bio_X509]
        RAND[RAND_bytes]
    end
    
    %% Function calls
    VP --> CFH
    VP --> LM
    VP --> GTL
    VP --> VS
    
    VS --> DHS
    VS --> CFH
    VS --> PEM
    VS --> X509
    VS --> EVP
    
    GTL --> IBL
    GTL --> IWL
    
    %% External calls
    DHS -.->|validates| VS
    
    style VP fill:#FFD700
    style VS fill:#FFD700
    style DHS fill:#87CEEB
```

### verifySignature() Call Flow

```mermaid
sequenceDiagram
    participant Caller
    participant VS as verifySignature
    participant DHS as decodeHexString
    participant CFH as calculateFileHash
    participant OpenSSL
    
    Caller->>VS: verifySignature(filePath, signature)
    VS->>CFH: calculateFileHash(filePath)
    CFH->>OpenSSL: EVP_DigestInit/Update/Final
    OpenSSL-->>CFH: SHA-256 hash
    CFH-->>VS: hexHash
    
    VS->>OpenSSL: PEM_read_bio_X509(certificate)
    OpenSSL-->>VS: X509 cert
    
    VS->>OpenSSL: X509_cmp_current_time(cert)
    OpenSSL-->>VS: expiration status
    
    VS->>OpenSSL: X509_get_pubkey(cert)
    OpenSSL-->>VS: public key
    
    VS->>DHS: decodeHexString(signature.signature)
    DHS-->>VS: signature bytes
    
    VS->>DHS: decodeHexString(hexHash)
    DHS-->>VS: hash bytes
    
    VS->>OpenSSL: EVP_DigestVerify(pubkey, sig, hash)
    OpenSSL-->>VS: verification result
    
    VS-->>Caller: bool (verified/failed)
```

## Manifest Database - Function Graph

```mermaid
graph TB
    subgraph "Public API"
        SM[storeManifest]
        GM[getManifest]
        VM[verifyManifest]
        VF[verifyFile]
        SF[storeFile]
        GF[getFile]
    end
    
    subgraph "Caching"
        CSV[cacheSignatureVerification]
        GCV[getCachedSignatureVerification]
        CD[cacheDownload]
        GCD[getCachedDownload]
    end
    
    subgraph "External Dependencies"
        PS[PluginSecurityVerifier]
        RDB[RocksDB]
        FS[Filesystem]
        RAND[OpenSSL RAND_bytes]
    end
    
    %% Manifest operations
    SM --> RDB
    GM --> RDB
    VM --> GCV
    VM --> PS
    VM --> CSV
    VM --> RAND
    VM --> FS
    
    VF --> GF
    VF --> PS
    VF --> FS
    
    SF --> RDB
    GF --> RDB
    
    %% Cache operations
    CSV --> RDB
    GCV --> RDB
    CD --> RDB
    GCD --> RDB
    
    style VM fill:#FFD700
    style VF fill:#FFD700
    style PS fill:#90EE90
```

### verifyManifest() Call Flow

```mermaid
sequenceDiagram
    participant Caller
    participant VM as verifyManifest
    participant Cache
    participant FS as Filesystem
    participant PS as PluginSecurityVerifier
    participant RAND as OpenSSL RAND
    
    Caller->>VM: verifyManifest(manifest)
    VM->>VM: calculateHash()
    
    alt Hash mismatch
        VM-->>Caller: false
    end
    
    VM->>Cache: getCachedSignatureVerification(hash)
    
    alt Cache hit
        Cache-->>VM: cached result
        VM-->>Caller: cached result
    end
    
    alt Cache miss
        VM->>RAND: RAND_bytes(16)
        RAND-->>VM: random bytes
        
        VM->>FS: Create temp file with random name
        FS-->>VM: temp file path
        
        VM->>FS: Write hash to temp file
        
        VM->>PS: verifySignature(tempPath, signature)
        PS-->>VM: verification result
        
        VM->>FS: Delete temp file
        
        VM->>Cache: cacheSignatureVerification(hash, result)
        
        VM-->>Caller: verification result
    end
```

## TrueTime - Function Graph

```mermaid
graph TB
    subgraph "Public API"
        NOW[now]
        WU[waitUntil]
        SN[syncNow]
        GU[getUncertainty]
        GD[getDrift]
    end
    
    subgraph "Sync Thread"
        STF[syncThreadFunc]
        PS[performSync]
        QNS[queryNTPServer]
    end
    
    subgraph "Internal"
        GST[getSystemTime]
        CU[calculateUncertainty]
    end
    
    subgraph "External"
        NTP[NTP Server]
        Socket[Socket API]
        DNS[getaddrinfo]
    end
    
    %% Public API calls
    NOW --> GST
    NOW --> CU
    WU --> NOW
    SN --> PS
    
    %% Sync operations
    STF --> PS
    PS --> QNS
    QNS --> DNS
    QNS --> Socket
    QNS --> NTP
    
    %% Internal operations
    CU -.->|reads| NOW
    
    style NOW fill:#FFD700
    style QNS fill:#FFD700
    style NTP fill:#FFA500
```

### queryNTPServer() Call Flow

```mermaid
sequenceDiagram
    participant Caller
    participant QNS as queryNTPServer
    participant DNS as getaddrinfo
    participant Socket
    participant NTP as NTP Server
    
    Caller->>QNS: queryNTPServer(server, offset&)
    
    QNS->>Socket: socket(AF_INET, SOCK_DGRAM)
    Socket-->>QNS: socket fd
    
    QNS->>Socket: setsockopt(SO_RCVTIMEO, 5s)
    
    QNS->>DNS: getaddrinfo(server, "123")
    DNS-->>QNS: server address
    
    Note over QNS: Record T1 (client transmit time)
    
    QNS->>Socket: sendto(NTP request packet)
    Socket->>NTP: NTP Request
    
    NTP-->>Socket: NTP Response (T2, T3)
    Socket-->>QNS: recvfrom(packet)
    
    Note over QNS: Record T4 (client receive time)
    
    QNS->>QNS: Validate timestamps (overflow check)
    QNS->>QNS: Calculate offset = ((T2-T1)+(T3-T4))/2
    
    QNS->>Socket: close(fd) [via RAII]
    
    QNS-->>Caller: true + offset
```

## Process Mining - Function Graph

```mermaid
graph TB
    subgraph "Discovery Algorithms"
        AM[runAlphaMiner]
        HM[runHeuristicMiner]
        IM[runInductiveMiner]
    end
    
    subgraph "Event Log Extraction"
        EEL[extractEventLog]
        EELG[extractEventLogFromGraph]
        EELR[extractEventLogFromReferences]
    end
    
    subgraph "Model Operations"
        SAPD[saveAsProcessDefinition]
        CC[checkConformance]
    end
    
    subgraph "AND Gateway Detection (New)"
        DAS[detectANDSplitGateways]
        DAJ[detectANDJoinGateways]
        RG[restructureGraph]
    end
    
    subgraph "External Dependencies"
        GI[GraphIndex]
        PG[ProcessGraph]
        SI[SecondaryIndex]
    end
    
    %% Discovery flows
    AM --> EELG
    AM --> DAS
    AM --> DAJ
    AM --> RG
    
    HM --> EELG
    IM --> EELG
    
    %% AND Gateway operations
    DAS -.->|identifies| RG
    DAJ -.->|identifies| RG
    
    %% Persistence
    SAPD --> PG
    
    %% Storage access
    EEL --> SI
    EELG --> GI
    
    style AM fill:#FFD700
    style DAS fill:#87CEEB
    style DAJ fill:#87CEEB
```

### runAlphaMiner() with AND Gateway Detection

```mermaid
sequenceDiagram
    participant Caller
    participant AM as runAlphaMiner
    participant EL as EventLog
    participant DFG as DFG Builder
    participant AGD as AND Gateway Detector
    
    Caller->>AM: runAlphaMiner(log, config)
    
    AM->>DFG: createDFG(log)
    DFG-->>AM: DirectlyFollowsGraph
    
    AM->>AM: identifyStartEndActivities()
    AM->>AM: computeCausalRelations()
    AM->>AM: computeParallelRelations()
    
    Note over AM: Create nodes for activities
    Note over AM: Create edges for causal relations
    
    AM->>AGD: detectANDSplitGateways(parallel relations)
    
    loop For each activity with multiple outgoing edges
        AGD->>AGD: Check if targets are parallel
        alt Targets are parallel
            AGD->>AGD: Create AND-split gateway
            AGD->>AGD: Redirect edges through gateway
        end
    end
    
    AM->>AGD: detectANDJoinGateways(parallel relations)
    
    loop For each activity with multiple incoming edges
        AGD->>AGD: Check if sources are parallel
        alt Sources are parallel
            AGD->>AGD: Create AND-join gateway
            AGD->>AGD: Redirect edges through gateway
        end
    end
    
    AM-->>Caller: DiscoveredProcess (with gateways)
```

## Cross-Component Integration

```mermaid
graph TB
    subgraph "HTTP API Layer"
        HR[Hot Reload Endpoint]
        PM_API[Process Mining Endpoint]
        Plugin_API[Plugin Load Endpoint]
    end
    
    subgraph "Business Logic"
        MD[ManifestDatabase]
        PM[ProcessMining]
        PL[PluginLoader]
    end
    
    subgraph "Security & Time"
        PS[PluginSecurityVerifier]
        TT[TrueTime]
    end
    
    subgraph "Distributed Systems"
        DT[DistributedTransaction]
        SR[ShardRouter]
        RM[ReplicationManager]
    end
    
    subgraph "Storage"
        RDB[RocksDB]
        GI[GraphIndex]
    end
    
    %% API to Business Logic
    HR --> MD
    PM_API --> PM
    Plugin_API --> PL
    
    %% Business Logic to Security
    MD --> PS
    PL --> PS
    
    %% Business Logic to Storage
    MD --> RDB
    PM --> GI
    
    %% Distributed Systems to Time
    DT --> TT
    SR --> TT
    RM --> TT
    
    style PS fill:#90EE90
    style MD fill:#90EE90
    style TT fill:#90EE90
    style PM fill:#90EE90
```

## Data Flow Diagram

```mermaid
flowchart LR
    subgraph "Input"
        Plugin[Plugin File]
        Manifest[Release Manifest]
        EventLog[Event Log]
        NTPReq[NTP Request]
    end
    
    subgraph "Processing"
        PS[PluginSecurityVerifier]
        MD[ManifestDatabase]
        PM[ProcessMining]
        TT[TrueTime]
    end
    
    subgraph "Output"
        Verified[Verified Plugin]
        Stored[Stored Manifest]
        Process[Process Model]
        Time[Time Interval]
    end
    
    Plugin --> PS
    PS --> Verified
    
    Manifest --> MD
    MD --> PS
    MD --> Stored
    
    EventLog --> PM
    PM --> Process
    
    NTPReq --> TT
    TT --> Time
    
    style PS fill:#90EE90
    style MD fill:#90EE90
    style PM fill:#90EE90
    style TT fill:#90EE90
```

## Call Frequency & Performance

```mermaid
graph TB
    subgraph "Hot Path (Frequent Calls)"
        TT_now[TrueTime::now]
        PS_cache[Signature Cache Lookup]
        PM_query[Process Mining Queries]
    end
    
    subgraph "Warm Path (Periodic Calls)"
        TT_sync[TrueTime::syncNow]
        MD_verify[ManifestDatabase::verifyManifest]
    end
    
    subgraph "Cold Path (Rare Calls)"
        PS_verify[PluginSecurityVerifier::verifySignature]
        PM_discover[ProcessMining::runAlphaMiner]
    end
    
    TT_now -.->|O(1)| A[Atomic reads]
    PS_cache -.->|O(1)| B[HashMap lookup]
    PM_query -.->|O(log n)| C[Index scan]
    
    TT_sync -.->|O(s)| D[s = servers]
    MD_verify -.->|O(n+c)| E[n = size, c = cache]
    
    PS_verify -.->|O(n)| F[n = file size]
    PM_discover -.->|O(e+a²)| G[e = events, a = activities]
    
    style TT_now fill:#90EE90
    style PS_cache fill:#90EE90
    style PM_query fill:#FFD700
```

---

**Legend:**
- 🟢 Green boxes: Modified components in this PR
- 🟡 Yellow boxes: Key functions with implementations
- 🔵 Blue boxes: Helper functions
- 🟠 Orange boxes: External systems
- Solid arrows: Direct function calls
- Dashed arrows: Data flow or usage
