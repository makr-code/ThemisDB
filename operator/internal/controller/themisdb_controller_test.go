/*
 * ThemisDB Kubernetes Operator – Controller Unit Tests
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

package controller_test

import (
	"testing"
	"time"

	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"

	vccv1alpha1 "github.com/makr-code/ThemisDB/operator/api/v1alpha1"
	"github.com/makr-code/ThemisDB/operator/internal/controller"
)

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

func makeCluster(replicas int32, mode vccv1alpha1.ReplicationMode, minSync int32) *vccv1alpha1.ThemisDB {
	return &vccv1alpha1.ThemisDB{
		ObjectMeta: metav1.ObjectMeta{Name: "test-cluster", Namespace: "default"},
		Spec: vccv1alpha1.ThemisDBSpec{
			Replicas: replicas,
			Version:  "1.0.0",
			Storage: vccv1alpha1.StorageConfig{
				Size:         "10Gi",
				StorageClass: "standard",
			},
			Replication: vccv1alpha1.ReplicationConfig{
				Mode:            mode,
				MinSyncReplicas: minSync,
				LeaderElection: vccv1alpha1.LeaderElectionConfig{
					ElectionTimeoutMinMs:     150,
					ElectionTimeoutMaxMs:     300,
					HeartbeatIntervalMs:      100,
					LeaderPreferencePriority: 1,
				},
				Failover: vccv1alpha1.FailoverConfig{
					Enabled:                   true,
					FailureDetectionTimeoutMs: 5000,
					MaxFailoverAttempts:       3,
					FailoverCooldownMs:        30000,
				},
				LagThresholds: vccv1alpha1.LagThresholdsConfig{
					DegradedLagMs:    5000,
					CriticalLagMs:    30000,
					ReadShiftEnabled: true,
				},
				WAL: vccv1alpha1.WALConfig{
					Compression:    true,
					SyncOnCommit:   true,
					RetentionHours: 168,
				},
				TopologyAware: vccv1alpha1.TopologyAwareConfig{
					SpreadAcrossZones: true,
					SpreadAcrossNodes: true,
				},
			},
		},
	}
}

func makeReadyPod(name, ns string, annotations map[string]string) corev1.Pod {
	pod := corev1.Pod{
		ObjectMeta: metav1.ObjectMeta{
			Name:        name,
			Namespace:   ns,
			Annotations: annotations,
		},
		Status: corev1.PodStatus{
			Conditions: []corev1.PodCondition{
				{Type: corev1.PodReady, Status: corev1.ConditionTrue},
			},
		},
	}
	return pod
}

func makeUnreadyPod(name, ns string, annotations map[string]string) corev1.Pod {
	pod := corev1.Pod{
		ObjectMeta: metav1.ObjectMeta{
			Name:        name,
			Namespace:   ns,
			Annotations: annotations,
		},
		Status: corev1.PodStatus{
			Conditions: []corev1.PodCondition{
				{Type: corev1.PodReady, Status: corev1.ConditionFalse},
			},
		},
	}
	return pod
}

// ─────────────────────────────────────────────────────────────────────────────
// countReady
// ─────────────────────────────────────────────────────────────────────────────

func TestCountReady_AllReady(t *testing.T) {
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", nil),
		makeReadyPod("pod-1", "default", nil),
		makeReadyPod("pod-2", "default", nil),
	}
	got := controller.CountReady(pods)
	if got != 3 {
		t.Fatalf("expected 3 ready, got %d", got)
	}
}

func TestCountReady_SomeUnready(t *testing.T) {
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", nil),
		makeUnreadyPod("pod-1", "default", nil),
		makeReadyPod("pod-2", "default", nil),
	}
	got := controller.CountReady(pods)
	if got != 2 {
		t.Fatalf("expected 2 ready, got %d", got)
	}
}

func TestCountReady_NoneReady(t *testing.T) {
	pods := []corev1.Pod{
		makeUnreadyPod("pod-0", "default", nil),
	}
	got := controller.CountReady(pods)
	if got != 0 {
		t.Fatalf("expected 0 ready, got %d", got)
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// clusterLabels
// ─────────────────────────────────────────────────────────────────────────────

func TestClusterLabels(t *testing.T) {
	labels := controller.ClusterLabels("my-cluster")
	tests := []struct{ key, want string }{
		{"app.kubernetes.io/name", "themisdb"},
		{"app.kubernetes.io/instance", "my-cluster"},
		{"app.kubernetes.io/managed-by", "themisdb-operator"},
	}
	for _, tt := range tests {
		if labels[tt.key] != tt.want {
			t.Errorf("label %q: want %q, got %q", tt.key, tt.want, labels[tt.key])
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// headlessSvcName / replConfigMapName
// ─────────────────────────────────────────────────────────────────────────────

func TestHeadlessSvcName(t *testing.T) {
	if controller.HeadlessSvcName("my-cluster") != "my-cluster-headless" {
		t.Fatal("unexpected headless service name")
	}
}

func TestReplConfigMapName(t *testing.T) {
	if controller.ReplConfigMapName("my-cluster") != "my-cluster-replication-config" {
		t.Fatal("unexpected configmap name")
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// derivePhase
// ─────────────────────────────────────────────────────────────────────────────

func TestDerivePhase_Running(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", nil),
		makeReadyPod("pod-1", "default", nil),
		makeReadyPod("pod-2", "default", nil),
	}
	topo := vccv1alpha1.ReplicationTopologyStatus{CurrentLeader: "pod-0"}
	phase := controller.DerivePhase(cluster, pods, topo)
	if phase != vccv1alpha1.PhaseRunning {
		t.Fatalf("expected Running, got %s", phase)
	}
}

func TestDerivePhase_Updating(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", nil),
		makeUnreadyPod("pod-1", "default", nil),
		makeReadyPod("pod-2", "default", nil),
	}
	topo := vccv1alpha1.ReplicationTopologyStatus{CurrentLeader: "pod-0"}
	phase := controller.DerivePhase(cluster, pods, topo)
	if phase != vccv1alpha1.PhaseUpdating {
		t.Fatalf("expected Updating, got %s", phase)
	}
}

func TestDerivePhase_Degraded(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", nil),
		makeUnreadyPod("pod-1", "default", nil),
	}
	topo := vccv1alpha1.ReplicationTopologyStatus{
		CurrentLeader:   "pod-0",
		LaggingReplicas: []string{"pod-1"},
	}
	phase := controller.DerivePhase(cluster, pods, topo)
	if phase != vccv1alpha1.PhaseDegraded {
		t.Fatalf("expected Degraded, got %s", phase)
	}
}

func TestDerivePhase_Pending(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	phase := controller.DerivePhase(cluster, nil, vccv1alpha1.ReplicationTopologyStatus{})
	if phase != vccv1alpha1.PhasePending {
		t.Fatalf("expected Pending, got %s", phase)
	}
}

func TestDerivePhase_Failed(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	pods := []corev1.Pod{
		makeUnreadyPod("pod-0", "default", nil),
		makeUnreadyPod("pod-1", "default", nil),
	}
	phase := controller.DerivePhase(cluster, pods, vccv1alpha1.ReplicationTopologyStatus{})
	if phase != vccv1alpha1.PhaseFailed {
		t.Fatalf("expected Failed, got %s", phase)
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// deriveTopology
// ─────────────────────────────────────────────────────────────────────────────

func TestDeriveTopology_IdentifiesLeader(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", map[string]string{
			"vcc.io/replication-role":   "LEADER",
			"vcc.io/replication-lag-ms": "0",
		}),
		makeReadyPod("pod-1", "default", map[string]string{
			"vcc.io/replication-role":   "FOLLOWER",
			"vcc.io/replication-lag-ms": "100",
		}),
		makeReadyPod("pod-2", "default", map[string]string{
			"vcc.io/replication-role":   "FOLLOWER",
			"vcc.io/replication-lag-ms": "200",
		}),
	}

	topo, requeue := controller.DeriveTopology(cluster, pods)
	if topo.CurrentLeader != "pod-0" {
		t.Errorf("expected leader pod-0, got %q", topo.CurrentLeader)
	}
	if len(topo.InSyncReplicas) != 3 {
		t.Errorf("expected 3 in-sync replicas, got %d", len(topo.InSyncReplicas))
	}
	if len(topo.LaggingReplicas) != 0 {
		t.Errorf("expected 0 lagging replicas, got %d", len(topo.LaggingReplicas))
	}
	if requeue {
		t.Error("did not expect fast requeue")
	}
}

func TestDeriveTopology_LaggingReplica(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 1)
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", map[string]string{
			"vcc.io/replication-role":   "LEADER",
			"vcc.io/replication-lag-ms": "0",
		}),
		makeReadyPod("pod-1", "default", map[string]string{
			"vcc.io/replication-role":   "FOLLOWER",
			"vcc.io/replication-lag-ms": "6000", // above 5000 degraded threshold
		}),
	}

	topo, _ := controller.DeriveTopology(cluster, pods)
	if len(topo.LaggingReplicas) != 1 || topo.LaggingReplicas[0] != "pod-1" {
		t.Errorf("expected pod-1 in lagging replicas, got %v", topo.LaggingReplicas)
	}
}

func TestDeriveTopology_FailedReplica_TriggersRequeue(t *testing.T) {
	cluster := makeCluster(2, vccv1alpha1.ReplicationModeAsync, 1)
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", map[string]string{
			"vcc.io/replication-role": "LEADER",
		}),
		makeUnreadyPod("pod-1", "default", map[string]string{
			"vcc.io/replication-role": "FOLLOWER",
		}),
	}

	_, requeue := controller.DeriveTopology(cluster, pods)
	if !requeue {
		t.Error("expected fast requeue due to failed follower")
	}
}

func TestDeriveTopology_NoLeaderAnnotation_PreviousLeaderPreserved(t *testing.T) {
	cluster := makeCluster(2, vccv1alpha1.ReplicationModeAsync, 1)
	cluster.Status.ReplicationTopology = vccv1alpha1.ReplicationTopologyStatus{
		CurrentLeader: "pod-0",
	}

	// Pods have no role annotations yet.
	pods := []corev1.Pod{
		makeReadyPod("pod-0", "default", nil),
		makeReadyPod("pod-1", "default", nil),
	}

	topo, requeue := controller.DeriveTopology(cluster, pods)
	if topo.CurrentLeader != "pod-0" {
		t.Errorf("expected previous leader pod-0 to be preserved, got %q", topo.CurrentLeader)
	}
	if !requeue {
		t.Error("expected fast requeue because no leader annotation was found")
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// buildConditions
// ─────────────────────────────────────────────────────────────────────────────

func TestBuildConditions_Running(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	topo := vccv1alpha1.ReplicationTopologyStatus{
		CurrentLeader:  "pod-0",
		InSyncReplicas: []string{"pod-0", "pod-1", "pod-2"},
	}
	conditions := controller.BuildConditions(cluster, vccv1alpha1.PhaseRunning, topo)

	if len(conditions) != 3 {
		t.Fatalf("expected 3 conditions, got %d", len(conditions))
	}
	for _, c := range conditions {
		switch c.Type {
		case vccv1alpha1.ConditionAvailable:
			if c.Status != "True" {
				t.Errorf("ConditionAvailable: expected True, got %s", c.Status)
			}
		case vccv1alpha1.ConditionTopologyReady:
			if c.Status != "True" {
				t.Errorf("ConditionTopologyReady: expected True, got %s", c.Status)
			}
		case vccv1alpha1.ConditionDegraded:
			if c.Status != "False" {
				t.Errorf("ConditionDegraded: expected False, got %s", c.Status)
			}
		}
	}
}

func TestBuildConditions_Degraded(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	topo := vccv1alpha1.ReplicationTopologyStatus{
		CurrentLeader:   "pod-0",
		InSyncReplicas:  []string{"pod-0"},
		LaggingReplicas: []string{"pod-1"},
	}
	conditions := controller.BuildConditions(cluster, vccv1alpha1.PhaseDegraded, topo)

	for _, c := range conditions {
		if c.Type == vccv1alpha1.ConditionDegraded && c.Status != "True" {
			t.Errorf("ConditionDegraded: expected True, got %s", c.Status)
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// buildReplicationConfigMap
// ─────────────────────────────────────────────────────────────────────────────

func TestBuildReplicationConfigMap_ContainsAllKeys(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	cm := controller.BuildReplicationConfigMap(cluster)

	requiredKeys := []string{
		"replication_mode", "min_sync_replicas",
		"election_timeout_min_ms", "election_timeout_max_ms",
		"heartbeat_interval_ms", "leader_priority",
		"failover_enabled", "failure_detection_timeout_ms",
		"max_failover_attempts", "failover_cooldown_ms",
		"degraded_lag_ms", "critical_lag_ms", "read_shift_enabled",
		"wal_compression", "wal_sync_on_commit", "wal_retention_hours",
		"topology_spread_across_zones", "topology_spread_across_nodes",
	}
	for _, key := range requiredKeys {
		if _, ok := cm.Data[key]; !ok {
			t.Errorf("configmap missing key: %s", key)
		}
	}
}

func TestBuildReplicationConfigMap_CorrectValues(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSync, 3)
	cm := controller.BuildReplicationConfigMap(cluster)

	if cm.Data["replication_mode"] != "SYNC" {
		t.Errorf("expected SYNC, got %q", cm.Data["replication_mode"])
	}
	if cm.Data["min_sync_replicas"] != "3" {
		t.Errorf("expected 3, got %q", cm.Data["min_sync_replicas"])
	}
	if cm.Data["failover_enabled"] != "true" {
		t.Errorf("expected failover_enabled=true")
	}
	if cm.Data["wal_compression"] != "true" {
		t.Errorf("expected wal_compression=true")
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// buildHeadlessService
// ─────────────────────────────────────────────────────────────────────────────

func TestBuildHeadlessService_IsHeadless(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	svc := controller.BuildHeadlessService(cluster)

	if svc.Spec.ClusterIP != "None" {
		t.Errorf("expected ClusterIP=None (headless), got %q", svc.Spec.ClusterIP)
	}
	if svc.Name != "test-cluster-headless" {
		t.Errorf("expected name test-cluster-headless, got %q", svc.Name)
	}
}

func TestBuildHeadlessService_HasExpectedPorts(t *testing.T) {
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	svc := controller.BuildHeadlessService(cluster)

	portNames := map[string]bool{}
	for _, p := range svc.Spec.Ports {
		portNames[p.Name] = true
	}
	for _, expected := range []string{"themisdb", "metrics"} {
		if !portNames[expected] {
			t.Errorf("missing port %q in headless service", expected)
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// buildStatefulSet
// ─────────────────────────────────────────────────────────────────────────────

func TestBuildStatefulSet_ReplicaCount(t *testing.T) {
	r := &controller.ThemisDBReconciler{}
	cluster := makeCluster(5, vccv1alpha1.ReplicationModeAsync, 1)
	sts := r.BuildStatefulSet(cluster)

	if *sts.Spec.Replicas != 5 {
		t.Errorf("expected 5 replicas, got %d", *sts.Spec.Replicas)
	}
}

func TestBuildStatefulSet_ContainsTopologyConstraints(t *testing.T) {
	r := &controller.ThemisDBReconciler{}
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	sts := r.BuildStatefulSet(cluster)

	if len(sts.Spec.Template.Spec.TopologySpreadConstraints) != 2 {
		t.Errorf("expected 2 topology spread constraints, got %d",
			len(sts.Spec.Template.Spec.TopologySpreadConstraints))
	}
}

func TestBuildStatefulSet_NoTopologyConstraints_WhenDisabled(t *testing.T) {
	r := &controller.ThemisDBReconciler{}
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	cluster.Spec.Replication.TopologyAware.SpreadAcrossZones = false
	cluster.Spec.Replication.TopologyAware.SpreadAcrossNodes = false
	sts := r.BuildStatefulSet(cluster)

	if len(sts.Spec.Template.Spec.TopologySpreadConstraints) != 0 {
		t.Errorf("expected 0 topology spread constraints, got %d",
			len(sts.Spec.Template.Spec.TopologySpreadConstraints))
	}
}

func TestBuildStatefulSet_ContainsReplicationEnvVars(t *testing.T) {
	r := &controller.ThemisDBReconciler{}
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSync, 3)
	sts := r.BuildStatefulSet(cluster)

	containers := sts.Spec.Template.Spec.Containers
	if len(containers) == 0 {
		t.Fatal("no containers in statefulset template")
	}

	envMap := map[string]string{}
	for _, e := range containers[0].Env {
		envMap[e.Name] = e.Value
	}

	tests := []struct{ key, want string }{
		{"THEMISDB_REPLICATION_MODE", "SYNC"},
		{"THEMISDB_MIN_SYNC_REPLICAS", "3"},
		{"THEMISDB_FAILOVER_ENABLED", "true"},
		{"THEMISDB_WAL_COMPRESSION", "true"},
	}
	for _, tt := range tests {
		if envMap[tt.key] != tt.want {
			t.Errorf("env %q: want %q, got %q", tt.key, tt.want, envMap[tt.key])
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// replicaHealthFromPod
// ─────────────────────────────────────────────────────────────────────────────

func TestReplicaHealthFromPod_ReadyIsHealthy(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", nil)
	health := controller.ReplicaHealthFromPod(&pod)
	if health != vccv1alpha1.ReplicaHealthHealthy {
		t.Errorf("expected HEALTHY, got %s", health)
	}
}

func TestReplicaHealthFromPod_UnreadyIsDegraded(t *testing.T) {
	pod := makeUnreadyPod("pod-0", "default", nil)
	health := controller.ReplicaHealthFromPod(&pod)
	if health != vccv1alpha1.ReplicaHealthDegraded {
		t.Errorf("expected DEGRADED, got %s", health)
	}
}

func TestReplicaHealthFromPod_DeletingIsFailed(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", nil)
	now := metav1.NewTime(time.Now())
	pod.DeletionTimestamp = &now
	health := controller.ReplicaHealthFromPod(&pod)
	if health != vccv1alpha1.ReplicaHealthFailed {
		t.Errorf("expected FAILED for terminating pod, got %s", health)
	}
}

func TestReplicaHealthFromPod_NoConditionIsUnknown(t *testing.T) {
	pod := corev1.Pod{
		ObjectMeta: metav1.ObjectMeta{Name: "pod-0"},
	}
	health := controller.ReplicaHealthFromPod(&pod)
	if health != vccv1alpha1.ReplicaHealthUnknown {
		t.Errorf("expected UNKNOWN, got %s", health)
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// lagMsFromAnnotation
// ─────────────────────────────────────────────────────────────────────────────

func TestLagMsFromAnnotation_Valid(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", map[string]string{
		"vcc.io/replication-lag-ms": "12345",
	})
	lag := controller.LagMsFromAnnotation(&pod)
	if lag != 12345 {
		t.Errorf("expected 12345, got %d", lag)
	}
}

func TestLagMsFromAnnotation_Missing(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", nil)
	lag := controller.LagMsFromAnnotation(&pod)
	if lag != 0 {
		t.Errorf("expected 0, got %d", lag)
	}
}
