/*
 * ThemisDB Kubernetes Operator – API Types
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/runtime/schema"
)

// GroupVersion is the group and version for ThemisDB API types.
var (
	GroupVersion  = schema.GroupVersion{Group: "vcc.io", Version: "v1alpha1"}
	SchemeBuilder = runtime.NewSchemeBuilder(addKnownTypes)
	AddToScheme   = SchemeBuilder.AddToScheme
)

func addKnownTypes(scheme *runtime.Scheme) error {
	scheme.AddKnownTypes(GroupVersion,
		&ThemisDB{},
		&ThemisDBList{},
	)
	metav1.AddToGroupVersion(scheme, GroupVersion)
	return nil
}

// ReplicationMode defines the replication consistency mode.
// +kubebuilder:validation:Enum=SYNC;SEMI_SYNC;ASYNC
type ReplicationMode string

const (
	ReplicationModeSync     ReplicationMode = "SYNC"
	ReplicationModeSemiSync ReplicationMode = "SEMI_SYNC"
	ReplicationModeAsync    ReplicationMode = "ASYNC"
)

// ReplicaRole describes the role of a replica within the replication topology.
// +kubebuilder:validation:Enum=LEADER;FOLLOWER;CANDIDATE;OBSERVER
type ReplicaRole string

const (
	ReplicaRoleLeader    ReplicaRole = "LEADER"
	ReplicaRoleFollower  ReplicaRole = "FOLLOWER"
	ReplicaRoleCandidate ReplicaRole = "CANDIDATE"
	ReplicaRoleObserver  ReplicaRole = "OBSERVER"
)

// ReplicaHealth describes the health state of a replica.
// +kubebuilder:validation:Enum=HEALTHY;DEGRADED;FAILED;UNKNOWN
type ReplicaHealth string

const (
	ReplicaHealthHealthy  ReplicaHealth = "HEALTHY"
	ReplicaHealthDegraded ReplicaHealth = "DEGRADED"
	ReplicaHealthFailed   ReplicaHealth = "FAILED"
	ReplicaHealthUnknown  ReplicaHealth = "UNKNOWN"
)

// LeaderElectionConfig configures the Raft-like leader election behaviour.
type LeaderElectionConfig struct {
	// ElectionTimeoutMinMs is the minimum election timeout in milliseconds.
	// +kubebuilder:default=150
	// +kubebuilder:validation:Minimum=150
	ElectionTimeoutMinMs int32 `json:"electionTimeoutMinMs,omitempty"`

	// ElectionTimeoutMaxMs is the maximum election timeout in milliseconds.
	// +kubebuilder:default=300
	// +kubebuilder:validation:Minimum=300
	ElectionTimeoutMaxMs int32 `json:"electionTimeoutMaxMs,omitempty"`

	// HeartbeatIntervalMs is the leader heartbeat interval in milliseconds.
	// +kubebuilder:default=100
	// +kubebuilder:validation:Minimum=50
	HeartbeatIntervalMs int32 `json:"heartbeatIntervalMs,omitempty"`

	// LeaderPreferencePriority is used to bias leader election towards specific
	// nodes; higher priority is preferred.
	// +kubebuilder:default=1
	// +kubebuilder:validation:Minimum=0
	LeaderPreferencePriority int32 `json:"leaderPreferencePriority,omitempty"`
}

// FailoverConfig controls the automatic failover policy.
type FailoverConfig struct {
	// Enabled enables automatic leader failover.
	// +kubebuilder:default=true
	Enabled bool `json:"enabled,omitempty"`

	// FailureDetectionTimeoutMs is the time (ms) before a silent node is
	// declared failed.
	// +kubebuilder:default=5000
	// +kubebuilder:validation:Minimum=1000
	FailureDetectionTimeoutMs int32 `json:"failureDetectionTimeoutMs,omitempty"`

	// MaxFailoverAttempts is the maximum number of failover attempts before
	// the operator raises a terminal error.
	// +kubebuilder:default=3
	// +kubebuilder:validation:Minimum=1
	MaxFailoverAttempts int32 `json:"maxFailoverAttempts,omitempty"`

	// FailoverCooldownMs is the minimum interval (ms) between consecutive
	// failovers.
	// +kubebuilder:default=30000
	// +kubebuilder:validation:Minimum=0
	FailoverCooldownMs int32 `json:"failoverCooldownMs,omitempty"`
}

// LagThresholdsConfig defines the replication lag thresholds used to trigger
// topology-aware routing decisions.
type LagThresholdsConfig struct {
	// DegradedLagMs is the lag (ms) above which a replica is considered
	// degraded.
	// +kubebuilder:default=5000
	// +kubebuilder:validation:Minimum=0
	DegradedLagMs int64 `json:"degradedLagMs,omitempty"`

	// CriticalLagMs is the lag (ms) above which a replica is excluded from
	// read traffic.
	// +kubebuilder:default=30000
	// +kubebuilder:validation:Minimum=0
	CriticalLagMs int64 `json:"criticalLagMs,omitempty"`

	// ReadShiftEnabled automatically shifts read traffic away from lagging
	// replicas.
	// +kubebuilder:default=true
	ReadShiftEnabled bool `json:"readShiftEnabled,omitempty"`
}

// WALConfig configures Write-Ahead Log behaviour.
type WALConfig struct {
	// Compression enables Zstd WAL compression.
	// +kubebuilder:default=true
	Compression bool `json:"compression,omitempty"`

	// SyncOnCommit fsyncs the WAL on every commit (durable but slower).
	// +kubebuilder:default=true
	SyncOnCommit bool `json:"syncOnCommit,omitempty"`

	// RetentionHours is how many hours to retain WAL files.
	// +kubebuilder:default=168
	// +kubebuilder:validation:Minimum=1
	RetentionHours int32 `json:"retentionHours,omitempty"`
}

// TopologyAwareConfig provides scheduling hints for topology-aware replica
// placement.
type TopologyAwareConfig struct {
	// SpreadAcrossZones spreads replicas across Kubernetes availability zones.
	// +kubebuilder:default=true
	SpreadAcrossZones bool `json:"spreadAcrossZones,omitempty"`

	// SpreadAcrossNodes spreads replicas across Kubernetes nodes.
	// +kubebuilder:default=true
	SpreadAcrossNodes bool `json:"spreadAcrossNodes,omitempty"`

	// PreferredDatacenters lists datacenters or regions preferred for replica
	// placement (informational; used as node affinity hints).
	PreferredDatacenters []string `json:"preferredDatacenters,omitempty"`
}

// ReplicationConfig is the replication topology block inside ThemisDBSpec.
type ReplicationConfig struct {
	// Mode is the replication consistency mode.
	// +kubebuilder:default=SEMI_SYNC
	Mode ReplicationMode `json:"mode,omitempty"`

	// MinSyncReplicas is the minimum number of in-sync replicas required for a
	// quorum write to succeed.
	// +kubebuilder:default=1
	// +kubebuilder:validation:Minimum=1
	MinSyncReplicas int32 `json:"minSyncReplicas,omitempty"`

	// LeaderElection configures Raft-like leader election.
	LeaderElection LeaderElectionConfig `json:"leaderElection,omitempty"`

	// Failover configures automatic leader failover.
	Failover FailoverConfig `json:"failover,omitempty"`

	// LagThresholds configures the replication lag thresholds.
	LagThresholds LagThresholdsConfig `json:"lagThresholds,omitempty"`

	// WAL configures the Write-Ahead Log.
	WAL WALConfig `json:"wal,omitempty"`

	// TopologyAware provides scheduling hints for replica placement.
	TopologyAware TopologyAwareConfig `json:"topologyAware,omitempty"`
}

// StorageConfig defines persistent storage for ThemisDB pods.
type StorageConfig struct {
	// Size is the requested PVC capacity.
	// +kubebuilder:default="100Gi"
	Size string `json:"size,omitempty"`

	// StorageClass is the Kubernetes StorageClass to use.
	// +kubebuilder:default="standard"
	StorageClass string `json:"storageClass,omitempty"`
}

// ResourceRequirements mirrors corev1.ResourceRequirements for simplicity.
type ResourceRequirements struct {
	Requests map[string]string `json:"requests,omitempty"`
	Limits   map[string]string `json:"limits,omitempty"`
}

// ShardingConfig defines the sharding topology.
type ShardingConfig struct {
	// Enabled enables sharding.
	// +kubebuilder:default=false
	Enabled bool `json:"enabled,omitempty"`

	// Shards is the number of shards.
	// +kubebuilder:default=3
	// +kubebuilder:validation:Minimum=1
	// +kubebuilder:validation:Maximum=100
	Shards int32 `json:"shards,omitempty"`

	// ReplicationFactor is the per-shard replica count.
	// +kubebuilder:default=2
	// +kubebuilder:validation:Minimum=1
	// +kubebuilder:validation:Maximum=5
	ReplicationFactor int32 `json:"replicationFactor,omitempty"`

	// GossipEnabled enables P2P gossip-based peer discovery.
	// +kubebuilder:default=false
	GossipEnabled bool `json:"gossipEnabled,omitempty"`

	// GossipInterval is the gossip period in seconds.
	// +kubebuilder:default=30
	GossipInterval int32 `json:"gossipInterval,omitempty"`
}

// SecurityConfig defines security settings for the cluster.
type SecurityConfig struct {
	// MTLS enables mutual TLS between nodes.
	// +kubebuilder:default=true
	MTLS bool `json:"mtls,omitempty"`

	// RBAC enables Kubernetes RBAC enforcement.
	// +kubebuilder:default=true
	RBAC bool `json:"rbac,omitempty"`

	// FieldEncryption enables field-level encryption at rest.
	// +kubebuilder:default=false
	FieldEncryption bool `json:"fieldEncryption,omitempty"`
}

// MonitoringConfig defines observability settings.
type MonitoringConfig struct {
	// Prometheus enables the Prometheus metrics endpoint.
	// +kubebuilder:default=true
	Prometheus bool `json:"prometheus,omitempty"`

	// Grafana deploys a Grafana dashboard sidecar.
	// +kubebuilder:default=false
	Grafana bool `json:"grafana,omitempty"`

	// OpenTelemetry enables OpenTelemetry tracing.
	// +kubebuilder:default=false
	OpenTelemetry bool `json:"opentelemetry,omitempty"`
}

// BackupConfig defines backup settings.
type BackupConfig struct {
	// Enabled enables scheduled backups.
	// +kubebuilder:default=false
	Enabled bool `json:"enabled,omitempty"`

	// Schedule is a cron expression for backup timing.
	// +kubebuilder:default="0 2 * * *"
	Schedule string `json:"schedule,omitempty"`

	// Retention is the number of backup snapshots to keep.
	// +kubebuilder:default=7
	Retention int32 `json:"retention,omitempty"`

	// Destination is an object-storage URL (s3://, gcs://, azure://).
	Destination string `json:"destination,omitempty"`
}

// ThemisDBSpec is the desired-state specification of a ThemisDB cluster.
type ThemisDBSpec struct {
	// Replicas is the total number of ThemisDB pods (leader + followers).
	// +kubebuilder:validation:Minimum=1
	// +kubebuilder:validation:Maximum=99
	Replicas int32 `json:"replicas"`

	// Version is the ThemisDB container image tag to deploy.
	// +kubebuilder:default="latest"
	Version string `json:"version,omitempty"`

	// Storage configures persistent volumes for each pod.
	Storage StorageConfig `json:"storage,omitempty"`

	// Resources sets CPU/memory requests and limits.
	Resources ResourceRequirements `json:"resources,omitempty"`

	// Replication configures the automated replication topology.
	Replication ReplicationConfig `json:"replication,omitempty"`

	// Sharding configures data distribution across shards.
	Sharding ShardingConfig `json:"sharding,omitempty"`

	// Security configures authentication and encryption.
	Security SecurityConfig `json:"security,omitempty"`

	// Monitoring configures observability integrations.
	Monitoring MonitoringConfig `json:"monitoring,omitempty"`

	// Backup configures scheduled backups.
	Backup BackupConfig `json:"backup,omitempty"`
}

// ReplicaHealthStatus holds the health snapshot for a single replica.
type ReplicaHealthStatus struct {
	// NodeID is the stable pod/node identifier.
	NodeID string `json:"nodeId"`

	// Role is the current replication role.
	Role ReplicaRole `json:"role"`

	// Health is the current health state.
	Health ReplicaHealth `json:"health"`

	// LagMs is the measured replication lag in milliseconds.
	LagMs int64 `json:"lagMs,omitempty"`

	// LastHeartbeat is when the operator last received a heartbeat.
	LastHeartbeat *metav1.Time `json:"lastHeartbeat,omitempty"`
}

// ReplicationTopologyStatus reports the live replication topology.
type ReplicationTopologyStatus struct {
	// CurrentLeader is the node ID of the current leader/primary.
	CurrentLeader string `json:"currentLeader,omitempty"`

	// LeaderTerm is the current Raft term.
	LeaderTerm int64 `json:"leaderTerm,omitempty"`

	// LastFailoverTime records when the most recent leader failover occurred.
	LastFailoverTime *metav1.Time `json:"lastFailoverTime,omitempty"`

	// FailoverCount is the cumulative number of leader failovers.
	FailoverCount int32 `json:"failoverCount,omitempty"`

	// InSyncReplicas lists replicas that are current with the leader.
	InSyncReplicas []string `json:"inSyncReplicas,omitempty"`

	// LaggingReplicas lists replicas that exceed the degraded lag threshold.
	LaggingReplicas []string `json:"laggingReplicas,omitempty"`

	// ReplicaHealth holds per-replica health snapshots.
	ReplicaHealth []ReplicaHealthStatus `json:"replicaHealth,omitempty"`
}

// ThemisDBConditionType is the condition type for ThemisDB status conditions.
type ThemisDBConditionType string

const (
	// ConditionAvailable indicates the cluster has the desired number of
	// ready replicas.
	ConditionAvailable ThemisDBConditionType = "Available"

	// ConditionTopologyReady indicates the replication topology has converged.
	ConditionTopologyReady ThemisDBConditionType = "TopologyReady"

	// ConditionFailoverInProgress indicates a leader failover is in progress.
	ConditionFailoverInProgress ThemisDBConditionType = "FailoverInProgress"

	// ConditionDegraded indicates at least one replica is lagging or unhealthy.
	ConditionDegraded ThemisDBConditionType = "Degraded"
)

// ThemisDBCondition represents a single status condition.
type ThemisDBCondition struct {
	Type               ThemisDBConditionType  `json:"type"`
	Status             string                 `json:"status"`
	Reason             string                 `json:"reason,omitempty"`
	Message            string                 `json:"message,omitempty"`
	LastTransitionTime metav1.Time            `json:"lastTransitionTime,omitempty"`
}

// ClusterPhase describes the high-level lifecycle phase.
// +kubebuilder:validation:Enum=Pending;Running;Updating;Degraded;Failed
type ClusterPhase string

const (
	PhasePending  ClusterPhase = "Pending"
	PhaseRunning  ClusterPhase = "Running"
	PhaseUpdating ClusterPhase = "Updating"
	PhaseDegraded ClusterPhase = "Degraded"
	PhaseFailed   ClusterPhase = "Failed"
)

// ThemisDBStatus is the observed state of a ThemisDB cluster.
type ThemisDBStatus struct {
	// Phase is the high-level lifecycle phase of the cluster.
	Phase ClusterPhase `json:"phase,omitempty"`

	// Replicas is the current total number of pods.
	Replicas int32 `json:"replicas,omitempty"`

	// ReadyReplicas is the number of pods that have passed readiness checks.
	ReadyReplicas int32 `json:"readyReplicas,omitempty"`

	// ReplicationTopology is the live replication topology status.
	ReplicationTopology ReplicationTopologyStatus `json:"replicationTopology,omitempty"`

	// Conditions is the list of status conditions.
	Conditions []ThemisDBCondition `json:"conditions,omitempty"`
}

// +kubebuilder:object:root=true
// +kubebuilder:subresource:status
// +kubebuilder:subresource:scale:specpath=.spec.replicas,statuspath=.status.replicas
// +kubebuilder:printcolumn:name="Replicas",type="integer",JSONPath=".spec.replicas"
// +kubebuilder:printcolumn:name="Ready",type="integer",JSONPath=".status.readyReplicas"
// +kubebuilder:printcolumn:name="Phase",type="string",JSONPath=".status.phase"
// +kubebuilder:printcolumn:name="Leader",type="string",JSONPath=".status.replicationTopology.currentLeader",priority=1
// +kubebuilder:printcolumn:name="Age",type="date",JSONPath=".metadata.creationTimestamp"

// ThemisDB is the Schema for the themisdbs API.
type ThemisDB struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   ThemisDBSpec   `json:"spec,omitempty"`
	Status ThemisDBStatus `json:"status,omitempty"`
}

// ThemisDBList contains a list of ThemisDB resources.
// +kubebuilder:object:root=true
type ThemisDBList struct {
	metav1.TypeMeta `json:",inline"`
	metav1.ListMeta `json:"metadata,omitempty"`
	Items           []ThemisDB `json:"items"`
}
