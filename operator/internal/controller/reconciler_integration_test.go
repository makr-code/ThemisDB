/*
 * ThemisDB Kubernetes Operator – Reconciler Integration Tests
 *
 * Uses controller-runtime's fake client to exercise the full Reconcile loop
 * and sub-reconcilers without a real Kubernetes cluster.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

package controller_test

import (
	"context"
	"testing"

	appsv1 "k8s.io/api/apps/v1"
	corev1 "k8s.io/api/core/v1"
	policyv1 "k8s.io/api/policy/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	clientgoscheme "k8s.io/client-go/kubernetes/scheme"
	ctrl "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/client/fake"

	vccv1alpha1 "github.com/makr-code/ThemisDB/operator/api/v1alpha1"
	"github.com/makr-code/ThemisDB/operator/internal/controller"
)

// ─────────────────────────────────────────────────────────────────────────────
// Scheme setup
// ─────────────────────────────────────────────────────────────────────────────

func buildScheme(t *testing.T) *runtime.Scheme {
	t.Helper()
	scheme := runtime.NewScheme()
	if err := clientgoscheme.AddToScheme(scheme); err != nil {
		t.Fatalf("clientgoscheme: %v", err)
	}
	if err := appsv1.AddToScheme(scheme); err != nil {
		t.Fatalf("appsv1: %v", err)
	}
	if err := corev1.AddToScheme(scheme); err != nil {
		t.Fatalf("corev1: %v", err)
	}
	if err := policyv1.AddToScheme(scheme); err != nil {
		t.Fatalf("policyv1: %v", err)
	}
	if err := vccv1alpha1.AddToScheme(scheme); err != nil {
		t.Fatalf("vccv1alpha1: %v", err)
	}
	return scheme
}

func newReconciler(t *testing.T, objs ...runtime.Object) (*controller.ThemisDBReconciler, *fake.ClientBuilder) {
	t.Helper()
	scheme := buildScheme(t)
	builder := fake.NewClientBuilder().WithScheme(scheme).WithStatusSubresource(&vccv1alpha1.ThemisDB{})
	if len(objs) > 0 {
		builder = builder.WithRuntimeObjects(objs...)
	}
	fakeClient := builder.Build()
	r := &controller.ThemisDBReconciler{
		Client: fakeClient,
		Scheme: scheme,
	}
	return r, builder
}

func defaultCluster() *vccv1alpha1.ThemisDB {
	return &vccv1alpha1.ThemisDB{
		TypeMeta: metav1.TypeMeta{
			APIVersion: "vcc.io/v1alpha1",
			Kind:       "ThemisDB",
		},
		ObjectMeta: metav1.ObjectMeta{
			Name:      "test-cluster",
			Namespace: "default",
		},
		Spec: vccv1alpha1.ThemisDBSpec{
			Replicas: 3,
			Version:  "1.0.0",
			Storage: vccv1alpha1.StorageConfig{
				Size:         "10Gi",
				StorageClass: "standard",
			},
			Resources: vccv1alpha1.ResourceRequirements{
				Requests: map[string]string{"cpu": "500m", "memory": "1Gi"},
				Limits:   map[string]string{"cpu": "1", "memory": "2Gi"},
			},
			Replication: vccv1alpha1.ReplicationConfig{
				Mode:            vccv1alpha1.ReplicationModeSemiSync,
				MinSyncReplicas: 2,
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
			Monitoring: vccv1alpha1.MonitoringConfig{Prometheus: true},
		},
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Reconcile – first pass (add finalizer)
// ─────────────────────────────────────────────────────────────────────────────

func TestReconcile_NotFound_ReturnsNil(t *testing.T) {
	r, _ := newReconciler(t)
	req := ctrl.Request{NamespacedName: types.NamespacedName{Name: "missing", Namespace: "default"}}
	result, err := r.Reconcile(context.Background(), req)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if result.Requeue || result.RequeueAfter != 0 {
		t.Errorf("expected empty result for not-found, got %+v", result)
	}
}

func TestReconcile_AddsFinalizer(t *testing.T) {
	cluster := defaultCluster()
	scheme := buildScheme(t)
	fakeClient := fake.NewClientBuilder().
		WithScheme(scheme).
		WithStatusSubresource(&vccv1alpha1.ThemisDB{}).
		WithObjects(cluster).
		Build()
	r := &controller.ThemisDBReconciler{Client: fakeClient, Scheme: scheme}

	req := ctrl.Request{NamespacedName: types.NamespacedName{Name: "test-cluster", Namespace: "default"}}
	result, err := r.Reconcile(context.Background(), req)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	// First pass adds finalizer and requeues.
	if !result.Requeue {
		t.Error("expected Requeue=true after adding finalizer")
	}

	// Verify finalizer was added.
	updated := &vccv1alpha1.ThemisDB{}
	if err := fakeClient.Get(context.Background(),
		types.NamespacedName{Name: "test-cluster", Namespace: "default"}, updated); err != nil {
		t.Fatalf("get cluster: %v", err)
	}
	found := false
	for _, f := range updated.Finalizers {
		if f == "vcc.io/themisdb-topology" {
			found = true
		}
	}
	if !found {
		t.Error("expected finalizer vcc.io/themisdb-topology to be present")
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Reconcile – full cycle (create owned resources)
// ─────────────────────────────────────────────────────────────────────────────

func clusterWithFinalizer() *vccv1alpha1.ThemisDB {
	c := defaultCluster()
	c.Finalizers = []string{"vcc.io/themisdb-topology"}
	return c
}

func TestReconcile_CreatesOwnedResources(t *testing.T) {
	cluster := clusterWithFinalizer()
	scheme := buildScheme(t)
	fakeClient := fake.NewClientBuilder().
		WithScheme(scheme).
		WithStatusSubresource(&vccv1alpha1.ThemisDB{}).
		WithObjects(cluster).
		Build()
	r := &controller.ThemisDBReconciler{Client: fakeClient, Scheme: scheme}

	req := ctrl.Request{NamespacedName: types.NamespacedName{Name: "test-cluster", Namespace: "default"}}
	_, err := r.Reconcile(context.Background(), req)
	if err != nil {
		t.Fatalf("reconcile: %v", err)
	}

	ctx := context.Background()

	// StatefulSet created
	sts := &appsv1.StatefulSet{}
	if err := fakeClient.Get(ctx, types.NamespacedName{Name: "test-cluster", Namespace: "default"}, sts); err != nil {
		t.Errorf("expected StatefulSet to be created: %v", err)
	}
	if *sts.Spec.Replicas != 3 {
		t.Errorf("expected 3 replicas in STS, got %d", *sts.Spec.Replicas)
	}

	// Headless Service created
	svc := &corev1.Service{}
	if err := fakeClient.Get(ctx, types.NamespacedName{Name: "test-cluster-headless", Namespace: "default"}, svc); err != nil {
		t.Errorf("expected headless Service to be created: %v", err)
	}
	if svc.Spec.ClusterIP != "None" {
		t.Errorf("expected ClusterIP=None, got %q", svc.Spec.ClusterIP)
	}

	// ConfigMap created
	cm := &corev1.ConfigMap{}
	if err := fakeClient.Get(ctx, types.NamespacedName{Name: "test-cluster-replication-config", Namespace: "default"}, cm); err != nil {
		t.Errorf("expected ConfigMap to be created: %v", err)
	}
	if cm.Data["replication_mode"] != "SEMI_SYNC" {
		t.Errorf("expected replication_mode=SEMI_SYNC, got %q", cm.Data["replication_mode"])
	}

	// PodDisruptionBudget created
	pdb := &policyv1.PodDisruptionBudget{}
	if err := fakeClient.Get(ctx, types.NamespacedName{Name: "test-cluster", Namespace: "default"}, pdb); err != nil {
		t.Errorf("expected PodDisruptionBudget to be created: %v", err)
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Reconcile – idempotency (second call updates existing resources)
// ─────────────────────────────────────────────────────────────────────────────

func TestReconcile_Idempotent(t *testing.T) {
	cluster := clusterWithFinalizer()
	scheme := buildScheme(t)
	fakeClient := fake.NewClientBuilder().
		WithScheme(scheme).
		WithStatusSubresource(&vccv1alpha1.ThemisDB{}).
		WithObjects(cluster).
		Build()
	r := &controller.ThemisDBReconciler{Client: fakeClient, Scheme: scheme}
	req := ctrl.Request{NamespacedName: types.NamespacedName{Name: "test-cluster", Namespace: "default"}}

	// First reconcile
	if _, err := r.Reconcile(context.Background(), req); err != nil {
		t.Fatalf("first reconcile: %v", err)
	}
	// Second reconcile – should succeed without error
	if _, err := r.Reconcile(context.Background(), req); err != nil {
		t.Fatalf("second reconcile: %v", err)
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Reconcile – PDB quorum math
// ─────────────────────────────────────────────────────────────────────────────

func TestReconcile_PDB_QuorumMath(t *testing.T) {
	tests := []struct {
		replicas       int32
		maxUnavailable int
	}{
		{1, 1},  // single node: maxUnavailable=1 (minimum)
		{2, 1},  // 2-node: floor(1/2)=0 → clamped to 1
		{3, 1},  // 3-node: floor(2/2)=1
		{5, 2},  // 5-node: floor(4/2)=2
		{7, 3},  // 7-node: floor(6/2)=3
	}
	for _, tt := range tests {
		t.Run("", func(t *testing.T) {
			cluster := clusterWithFinalizer()
			cluster.Spec.Replicas = tt.replicas

			scheme := buildScheme(t)
			fakeClient := fake.NewClientBuilder().
				WithScheme(scheme).
				WithStatusSubresource(&vccv1alpha1.ThemisDB{}).
				WithObjects(cluster).
				Build()
			r := &controller.ThemisDBReconciler{Client: fakeClient, Scheme: scheme}
			req := ctrl.Request{NamespacedName: types.NamespacedName{Name: "test-cluster", Namespace: "default"}}
			if _, err := r.Reconcile(context.Background(), req); err != nil {
				t.Fatalf("reconcile: %v", err)
			}

			pdb := &policyv1.PodDisruptionBudget{}
			if err := fakeClient.Get(context.Background(),
				types.NamespacedName{Name: "test-cluster", Namespace: "default"}, pdb); err != nil {
				t.Fatalf("get PDB: %v", err)
			}
			got := pdb.Spec.MaxUnavailable.IntValue()
			if got != tt.maxUnavailable {
				t.Errorf("replicas=%d: want maxUnavailable=%d, got %d", tt.replicas, tt.maxUnavailable, got)
			}
		})
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Reconcile – deletion
// ─────────────────────────────────────────────────────────────────────────────

func TestReconcile_Deletion_RemovesFinalizer(t *testing.T) {
	now := metav1.Now()
	cluster := clusterWithFinalizer()
	cluster.DeletionTimestamp = &now

	scheme := buildScheme(t)
	fakeClient := fake.NewClientBuilder().
		WithScheme(scheme).
		WithStatusSubresource(&vccv1alpha1.ThemisDB{}).
		WithObjects(cluster).
		Build()
	r := &controller.ThemisDBReconciler{Client: fakeClient, Scheme: scheme}
	req := ctrl.Request{NamespacedName: types.NamespacedName{Name: "test-cluster", Namespace: "default"}}
	_, err := r.Reconcile(context.Background(), req)
	if err != nil {
		t.Fatalf("reconcile during deletion: %v", err)
	}
	// After the last finalizer is removed the fake client garbage-collects the
	// object.  Either the object is gone (NotFound) or it has no finalizers.
	updated := &vccv1alpha1.ThemisDB{}
	getErr := fakeClient.Get(context.Background(),
		types.NamespacedName{Name: "test-cluster", Namespace: "default"}, updated)
	if getErr == nil {
		// Object still present – verify finalizer was stripped.
		for _, f := range updated.Finalizers {
			if f == "vcc.io/themisdb-topology" {
				t.Error("expected finalizer to be removed")
			}
		}
	}
	// getErr != nil (NotFound) is also acceptable: object was gc'd after finalizer removal.
}

// ─────────────────────────────────────────────────────────────────────────────
// Reconcile – topology status updated from pods
// ─────────────────────────────────────────────────────────────────────────────

func TestReconcile_TopologyStatus_UpdatedFromPods(t *testing.T) {
	cluster := clusterWithFinalizer()
	labels := map[string]string{
		"app.kubernetes.io/name":       "themisdb",
		"app.kubernetes.io/instance":   "test-cluster",
		"app.kubernetes.io/managed-by": "themisdb-operator",
	}
	pods := []runtime.Object{
		&corev1.Pod{
			ObjectMeta: metav1.ObjectMeta{
				Name:      "test-cluster-0",
				Namespace: "default",
				Labels:    labels,
				Annotations: map[string]string{
					"vcc.io/replication-role":   "LEADER",
					"vcc.io/replication-lag-ms": "0",
				},
			},
			Status: corev1.PodStatus{
				Conditions: []corev1.PodCondition{
					{Type: corev1.PodReady, Status: corev1.ConditionTrue},
				},
			},
		},
		&corev1.Pod{
			ObjectMeta: metav1.ObjectMeta{
				Name:      "test-cluster-1",
				Namespace: "default",
				Labels:    labels,
				Annotations: map[string]string{
					"vcc.io/replication-role":   "FOLLOWER",
					"vcc.io/replication-lag-ms": "150",
				},
			},
			Status: corev1.PodStatus{
				Conditions: []corev1.PodCondition{
					{Type: corev1.PodReady, Status: corev1.ConditionTrue},
				},
			},
		},
	}

	scheme := buildScheme(t)
	builder := fake.NewClientBuilder().
		WithScheme(scheme).
		WithStatusSubresource(&vccv1alpha1.ThemisDB{}).
		WithObjects(cluster)
	for _, p := range pods {
		builder = builder.WithRuntimeObjects(p)
	}
	fakeClient := builder.Build()
	r := &controller.ThemisDBReconciler{Client: fakeClient, Scheme: scheme}
	req := ctrl.Request{NamespacedName: types.NamespacedName{Name: "test-cluster", Namespace: "default"}}
	if _, err := r.Reconcile(context.Background(), req); err != nil {
		t.Fatalf("reconcile: %v", err)
	}

	updated := &vccv1alpha1.ThemisDB{}
	if err := fakeClient.Get(context.Background(),
		types.NamespacedName{Name: "test-cluster", Namespace: "default"}, updated); err != nil {
		t.Fatalf("get cluster: %v", err)
	}
	if updated.Status.ReplicationTopology.CurrentLeader != "test-cluster-0" {
		t.Errorf("expected leader test-cluster-0, got %q", updated.Status.ReplicationTopology.CurrentLeader)
	}
	if updated.Status.Replicas != 2 {
		t.Errorf("expected replicas=2, got %d", updated.Status.Replicas)
	}
	if updated.Status.ReadyReplicas != 2 {
		t.Errorf("expected readyReplicas=2, got %d", updated.Status.ReadyReplicas)
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// replicaRoleFromAnnotation – additional role coverage
// ─────────────────────────────────────────────────────────────────────────────

func TestReplicaRoleFromAnnotation_Candidate(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", map[string]string{
		"vcc.io/replication-role": "CANDIDATE",
	})
	role := controller.ReplicaRoleFromAnnotation(&pod)
	if role != vccv1alpha1.ReplicaRoleCandidate {
		t.Errorf("expected CANDIDATE, got %s", role)
	}
}

func TestReplicaRoleFromAnnotation_Observer(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", map[string]string{
		"vcc.io/replication-role": "OBSERVER",
	})
	role := controller.ReplicaRoleFromAnnotation(&pod)
	if role != vccv1alpha1.ReplicaRoleObserver {
		t.Errorf("expected OBSERVER, got %s", role)
	}
}

func TestReplicaRoleFromAnnotation_UnknownDefaultsToFollower(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", map[string]string{
		"vcc.io/replication-role": "INVALID_ROLE",
	})
	role := controller.ReplicaRoleFromAnnotation(&pod)
	if role != vccv1alpha1.ReplicaRoleFollower {
		t.Errorf("expected FOLLOWER default for unknown role, got %s", role)
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// buildStatefulSet – resources set
// ─────────────────────────────────────────────────────────────────────────────

func TestBuildStatefulSet_WithResources(t *testing.T) {
	r := &controller.ThemisDBReconciler{}
	cluster := makeCluster(3, vccv1alpha1.ReplicationModeSemiSync, 2)
	cluster.Spec.Resources = vccv1alpha1.ResourceRequirements{
		Requests: map[string]string{"cpu": "500m", "memory": "1Gi"},
		Limits:   map[string]string{"cpu": "2", "memory": "4Gi"},
	}
	sts := r.BuildStatefulSet(cluster)
	containers := sts.Spec.Template.Spec.Containers
	if len(containers) == 0 {
		t.Fatal("no containers")
	}
	res := containers[0].Resources
	if res.Requests == nil {
		t.Error("expected resource Requests to be set")
	}
	if res.Limits == nil {
		t.Error("expected resource Limits to be set")
	}
}

func TestBuildStatefulSet_DefaultStorageSize(t *testing.T) {
	r := &controller.ThemisDBReconciler{}
	cluster := makeCluster(1, vccv1alpha1.ReplicationModeAsync, 1)
	cluster.Spec.Storage.Size = "" // empty should default to 100Gi
	sts := r.BuildStatefulSet(cluster)
	if len(sts.Spec.VolumeClaimTemplates) == 0 {
		t.Fatal("no VolumeClaimTemplates")
	}
	storageReq := sts.Spec.VolumeClaimTemplates[0].Spec.Resources.Requests[corev1.ResourceStorage]
	if storageReq.String() != "100Gi" {
		t.Errorf("expected 100Gi default storage, got %s", storageReq.String())
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// ReplicaRoleFromAnnotation – exported helper
// ─────────────────────────────────────────────────────────────────────────────

func TestReplicaRoleFromAnnotation_Leader(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", map[string]string{
		"vcc.io/replication-role": "LEADER",
	})
	role := controller.ReplicaRoleFromAnnotation(&pod)
	if role != vccv1alpha1.ReplicaRoleLeader {
		t.Errorf("expected LEADER, got %s", role)
	}
}

func TestReplicaRoleFromAnnotation_Follower(t *testing.T) {
	pod := makeReadyPod("pod-0", "default", map[string]string{
		"vcc.io/replication-role": "FOLLOWER",
	})
	role := controller.ReplicaRoleFromAnnotation(&pod)
	if role != vccv1alpha1.ReplicaRoleFollower {
		t.Errorf("expected FOLLOWER, got %s", role)
	}
}
