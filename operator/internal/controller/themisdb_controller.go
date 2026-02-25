/*
 * ThemisDB Kubernetes Operator – Replication Topology Controller
 *
 * Reconciles ThemisDB custom resources and manages the replication topology
 * by owning a StatefulSet, a headless Service, and per-replica PodDisruptionBudgets.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

package controller

import (
	"context"
	"fmt"
	"time"

	appsv1 "k8s.io/api/apps/v1"
	corev1 "k8s.io/api/core/v1"
	policyv1 "k8s.io/api/policy/v1"
	apierrors "k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/api/resource"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	"k8s.io/apimachinery/pkg/util/intstr"
	ctrl "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller/controllerutil"
	"sigs.k8s.io/controller-runtime/pkg/log"

	vccv1alpha1 "github.com/makr-code/ThemisDB/operator/api/v1alpha1"
)

const (
	// finalizerName is placed on ThemisDB resources while the operator is
	// managing them to guarantee clean-up on deletion.
	finalizerName = "vcc.io/themisdb-topology"

	// defaultRequeueAfter is how long the controller waits before re-queuing
	// during non-error reconciliation loops (topology health polling).
	defaultRequeueAfter = 30 * time.Second

	// failoverRequeueAfter is the shorter requeue interval used while a
	// failover is in progress.
	failoverRequeueAfter = 5 * time.Second

	// themisDBPort is the default TCP port used for ThemisDB communication.
	themisDBPort = 7070

	// metricsPort is the Prometheus metrics port.
	metricsPort = 4318
)

// ThemisDBReconciler reconciles ThemisDB objects.
type ThemisDBReconciler struct {
	client.Client
	Scheme *runtime.Scheme
}

// +kubebuilder:rbac:groups=vcc.io,resources=themisdbs,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=vcc.io,resources=themisdbs/status,verbs=get;update;patch
// +kubebuilder:rbac:groups=vcc.io,resources=themisdbs/finalizers,verbs=update
// +kubebuilder:rbac:groups=apps,resources=statefulsets,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=services,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=pods,verbs=get;list;watch
// +kubebuilder:rbac:groups=core,resources=configmaps,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=policy,resources=poddisruptionbudgets,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=events,verbs=create;patch

// Reconcile is the main reconciliation loop for ThemisDB resources.
func (r *ThemisDBReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	logger := log.FromContext(ctx)

	// ── 1. Fetch the ThemisDB resource ────────────────────────────────────
	cluster := &vccv1alpha1.ThemisDB{}
	if err := r.Get(ctx, req.NamespacedName, cluster); err != nil {
		if apierrors.IsNotFound(err) {
			return ctrl.Result{}, nil
		}
		return ctrl.Result{}, fmt.Errorf("get ThemisDB %s: %w", req.NamespacedName, err)
	}

	// ── 2. Deletion handling ───────────────────────────────────────────────
	if !cluster.DeletionTimestamp.IsZero() {
		return r.reconcileDeletion(ctx, cluster)
	}

	// ── 3. Ensure finalizer ───────────────────────────────────────────────
	if !controllerutil.ContainsFinalizer(cluster, finalizerName) {
		controllerutil.AddFinalizer(cluster, finalizerName)
		if err := r.Update(ctx, cluster); err != nil {
			return ctrl.Result{}, fmt.Errorf("add finalizer: %w", err)
		}
		return ctrl.Result{Requeue: true}, nil
	}

	// ── 4. Reconcile owned resources ──────────────────────────────────────
	if err := r.reconcileHeadlessService(ctx, cluster); err != nil {
		return ctrl.Result{}, err
	}
	if err := r.reconcileStatefulSet(ctx, cluster); err != nil {
		return ctrl.Result{}, err
	}
	if err := r.reconcilePodDisruptionBudget(ctx, cluster); err != nil {
		return ctrl.Result{}, err
	}
	if err := r.reconcileReplicationConfigMap(ctx, cluster); err != nil {
		return ctrl.Result{}, err
	}

	// ── 5. Observe topology & update status ──────────────────────────────
	requeue, err := r.reconcileTopologyStatus(ctx, cluster)
	if err != nil {
		logger.Error(err, "topology status reconciliation failed")
		return ctrl.Result{RequeueAfter: failoverRequeueAfter}, nil
	}

	if requeue {
		return ctrl.Result{RequeueAfter: failoverRequeueAfter}, nil
	}
	return ctrl.Result{RequeueAfter: defaultRequeueAfter}, nil
}

// reconcileDeletion removes the finalizer once owned resources have been
// cleaned up (owner references take care of garbage-collecting the StatefulSet
// and Services; we just need to remove the finalizer).
func (r *ThemisDBReconciler) reconcileDeletion(ctx context.Context, cluster *vccv1alpha1.ThemisDB) (ctrl.Result, error) {
	if controllerutil.ContainsFinalizer(cluster, finalizerName) {
		controllerutil.RemoveFinalizer(cluster, finalizerName)
		if err := r.Update(ctx, cluster); err != nil {
			return ctrl.Result{}, fmt.Errorf("remove finalizer: %w", err)
		}
	}
	return ctrl.Result{}, nil
}

// ─────────────────────────────────────────────────────────────────────────────
// Headless Service
// ─────────────────────────────────────────────────────────────────────────────

func (r *ThemisDBReconciler) reconcileHeadlessService(ctx context.Context, cluster *vccv1alpha1.ThemisDB) error {
	svc := &corev1.Service{}
	name := types.NamespacedName{Name: headlessSvcName(cluster.Name), Namespace: cluster.Namespace}

	err := r.Get(ctx, name, svc)
	if apierrors.IsNotFound(err) {
		svc = buildHeadlessService(cluster)
		if err := controllerutil.SetControllerReference(cluster, svc, r.Scheme); err != nil {
			return err
		}
		return r.Create(ctx, svc)
	}
	if err != nil {
		return fmt.Errorf("get headless service: %w", err)
	}

	// Ensure labels and ports are up to date.
	desired := buildHeadlessService(cluster)
	svc.Spec.Ports = desired.Spec.Ports
	svc.Spec.Selector = desired.Spec.Selector
	return r.Update(ctx, svc)
}

func buildHeadlessService(cluster *vccv1alpha1.ThemisDB) *corev1.Service {
	return &corev1.Service{
		ObjectMeta: metav1.ObjectMeta{
			Name:      headlessSvcName(cluster.Name),
			Namespace: cluster.Namespace,
			Labels:    clusterLabels(cluster.Name),
		},
		Spec: corev1.ServiceSpec{
			ClusterIP: "None",
			Selector:  clusterLabels(cluster.Name),
			Ports: []corev1.ServicePort{
				{
					Name:       "themisdb",
					Port:       themisDBPort,
					TargetPort: intstr.FromInt(themisDBPort),
					Protocol:   corev1.ProtocolTCP,
				},
				{
					Name:       "metrics",
					Port:       metricsPort,
					TargetPort: intstr.FromInt(metricsPort),
					Protocol:   corev1.ProtocolTCP,
				},
			},
		},
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// StatefulSet
// ─────────────────────────────────────────────────────────────────────────────

func (r *ThemisDBReconciler) reconcileStatefulSet(ctx context.Context, cluster *vccv1alpha1.ThemisDB) error {
	sts := &appsv1.StatefulSet{}
	name := types.NamespacedName{Name: cluster.Name, Namespace: cluster.Namespace}

	err := r.Get(ctx, name, sts)
	if apierrors.IsNotFound(err) {
		sts = r.buildStatefulSet(cluster)
		if err := controllerutil.SetControllerReference(cluster, sts, r.Scheme); err != nil {
			return err
		}
		return r.Create(ctx, sts)
	}
	if err != nil {
		return fmt.Errorf("get statefulset: %w", err)
	}

	// Apply rolling-update changes: replicas, image tag, resource requests.
	desired := r.buildStatefulSet(cluster)
	sts.Spec.Replicas = desired.Spec.Replicas
	sts.Spec.Template.Spec.Containers = desired.Spec.Template.Spec.Containers
	sts.Spec.Template.Spec.TopologySpreadConstraints = desired.Spec.Template.Spec.TopologySpreadConstraints
	return r.Update(ctx, sts)
}

func (r *ThemisDBReconciler) buildStatefulSet(cluster *vccv1alpha1.ThemisDB) *appsv1.StatefulSet {
	replicas := cluster.Spec.Replicas
	image := fmt.Sprintf("ghcr.io/makr-code/themisdb:%s", cluster.Spec.Version)

	repl := cluster.Spec.Replication

	// ── Environment variables that configure replication inside the pod ──
	envVars := []corev1.EnvVar{
		{Name: "THEMISDB_REPLICATION_MODE", Value: string(repl.Mode)},
		{Name: "THEMISDB_MIN_SYNC_REPLICAS", Value: fmt.Sprintf("%d", repl.MinSyncReplicas)},
		{Name: "THEMISDB_ELECTION_TIMEOUT_MIN_MS", Value: fmt.Sprintf("%d", repl.LeaderElection.ElectionTimeoutMinMs)},
		{Name: "THEMISDB_ELECTION_TIMEOUT_MAX_MS", Value: fmt.Sprintf("%d", repl.LeaderElection.ElectionTimeoutMaxMs)},
		{Name: "THEMISDB_HEARTBEAT_INTERVAL_MS", Value: fmt.Sprintf("%d", repl.LeaderElection.HeartbeatIntervalMs)},
		{Name: "THEMISDB_LEADER_PRIORITY", Value: fmt.Sprintf("%d", repl.LeaderElection.LeaderPreferencePriority)},
		{Name: "THEMISDB_FAILOVER_ENABLED", Value: fmt.Sprintf("%v", repl.Failover.Enabled)},
		{Name: "THEMISDB_FAILURE_DETECTION_TIMEOUT_MS", Value: fmt.Sprintf("%d", repl.Failover.FailureDetectionTimeoutMs)},
		{Name: "THEMISDB_DEGRADED_LAG_MS", Value: fmt.Sprintf("%d", repl.LagThresholds.DegradedLagMs)},
		{Name: "THEMISDB_CRITICAL_LAG_MS", Value: fmt.Sprintf("%d", repl.LagThresholds.CriticalLagMs)},
		{Name: "THEMISDB_READ_SHIFT_ENABLED", Value: fmt.Sprintf("%v", repl.LagThresholds.ReadShiftEnabled)},
		{Name: "THEMISDB_WAL_COMPRESSION", Value: fmt.Sprintf("%v", repl.WAL.Compression)},
		{Name: "THEMISDB_WAL_SYNC_ON_COMMIT", Value: fmt.Sprintf("%v", repl.WAL.SyncOnCommit)},
		{Name: "THEMISDB_WAL_RETENTION_HOURS", Value: fmt.Sprintf("%d", repl.WAL.RetentionHours)},
		// Pod identity for peer-discovery via the headless service.
		{
			Name: "THEMISDB_NODE_ID",
			ValueFrom: &corev1.EnvVarSource{
				FieldRef: &corev1.ObjectFieldSelector{FieldPath: "metadata.name"},
			},
		},
		{
			Name:  "THEMISDB_HEADLESS_SERVICE",
			Value: fmt.Sprintf("%s.%s.svc.cluster.local", headlessSvcName(cluster.Name), cluster.Namespace),
		},
	}

	// ── Resource requests / limits ──────────────────────────────────────
	resources := corev1.ResourceRequirements{}
	if cluster.Spec.Resources.Requests != nil {
		resources.Requests = corev1.ResourceList{}
		for k, v := range cluster.Spec.Resources.Requests {
			resources.Requests[corev1.ResourceName(k)] = mustParseQuantity(v)
		}
	}
	if cluster.Spec.Resources.Limits != nil {
		resources.Limits = corev1.ResourceList{}
		for k, v := range cluster.Spec.Resources.Limits {
			resources.Limits[corev1.ResourceName(k)] = mustParseQuantity(v)
		}
	}

	// ── Topology spread constraints for zone/node awareness ─────────────
	var topologyConstraints []corev1.TopologySpreadConstraint
	if repl.TopologyAware.SpreadAcrossZones {
		topologyConstraints = append(topologyConstraints, corev1.TopologySpreadConstraint{
			MaxSkew:           1,
			TopologyKey:       "topology.kubernetes.io/zone",
			WhenUnsatisfiable: corev1.ScheduleAnyway,
			LabelSelector: &metav1.LabelSelector{
				MatchLabels: clusterLabels(cluster.Name),
			},
		})
	}
	if repl.TopologyAware.SpreadAcrossNodes {
		topologyConstraints = append(topologyConstraints, corev1.TopologySpreadConstraint{
			MaxSkew:           1,
			TopologyKey:       "kubernetes.io/hostname",
			WhenUnsatisfiable: corev1.ScheduleAnyway,
			LabelSelector: &metav1.LabelSelector{
				MatchLabels: clusterLabels(cluster.Name),
			},
		})
	}

	// ── PVC template ────────────────────────────────────────────────────
	storageClass := cluster.Spec.Storage.StorageClass
	storageSize := cluster.Spec.Storage.Size
	if storageSize == "" {
		storageSize = "100Gi"
	}

	return &appsv1.StatefulSet{
		ObjectMeta: metav1.ObjectMeta{
			Name:      cluster.Name,
			Namespace: cluster.Namespace,
			Labels:    clusterLabels(cluster.Name),
		},
		Spec: appsv1.StatefulSetSpec{
			Replicas:    &replicas,
			ServiceName: headlessSvcName(cluster.Name),
			Selector: &metav1.LabelSelector{
				MatchLabels: clusterLabels(cluster.Name),
			},
			PodManagementPolicy: appsv1.ParallelPodManagement,
			UpdateStrategy: appsv1.StatefulSetUpdateStrategy{
				Type: appsv1.RollingUpdateStatefulSetStrategyType,
			},
			Template: corev1.PodTemplateSpec{
				ObjectMeta: metav1.ObjectMeta{
					Labels: clusterLabels(cluster.Name),
					Annotations: map[string]string{
						"prometheus.io/scrape": fmt.Sprintf("%v", cluster.Spec.Monitoring.Prometheus),
						"prometheus.io/port":   fmt.Sprintf("%d", metricsPort),
					},
				},
				Spec: corev1.PodSpec{
					TopologySpreadConstraints: topologyConstraints,
					Containers: []corev1.Container{
						{
							Name:      "themisdb",
							Image:     image,
							Resources: resources,
							Env:       envVars,
							Ports: []corev1.ContainerPort{
								{Name: "themisdb", ContainerPort: themisDBPort, Protocol: corev1.ProtocolTCP},
								{Name: "metrics", ContainerPort: metricsPort, Protocol: corev1.ProtocolTCP},
							},
							VolumeMounts: []corev1.VolumeMount{
								{Name: "data", MountPath: "/var/lib/themisdb"},
							},
							LivenessProbe: &corev1.Probe{
								ProbeHandler: corev1.ProbeHandler{
									HTTPGet: &corev1.HTTPGetAction{
										Path: "/health",
										Port: intstr.FromInt(themisDBPort),
									},
								},
								InitialDelaySeconds: 30,
								PeriodSeconds:       10,
								TimeoutSeconds:      5,
								FailureThreshold:    3,
							},
							ReadinessProbe: &corev1.Probe{
								ProbeHandler: corev1.ProbeHandler{
									HTTPGet: &corev1.HTTPGetAction{
										Path: "/ready",
										Port: intstr.FromInt(themisDBPort),
									},
								},
								InitialDelaySeconds: 10,
								PeriodSeconds:       5,
								TimeoutSeconds:      3,
								FailureThreshold:    3,
							},
						},
					},
				},
			},
			VolumeClaimTemplates: []corev1.PersistentVolumeClaim{
				{
					ObjectMeta: metav1.ObjectMeta{Name: "data"},
					Spec: corev1.PersistentVolumeClaimSpec{
						AccessModes:      []corev1.PersistentVolumeAccessMode{corev1.ReadWriteOnce},
						StorageClassName: &storageClass,
						Resources: corev1.VolumeResourceRequirements{
							Requests: corev1.ResourceList{
								corev1.ResourceStorage: mustParseQuantity(storageSize),
							},
						},
					},
				},
			},
		},
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// PodDisruptionBudget
// ─────────────────────────────────────────────────────────────────────────────

func (r *ThemisDBReconciler) reconcilePodDisruptionBudget(ctx context.Context, cluster *vccv1alpha1.ThemisDB) error {
	pdb := &policyv1.PodDisruptionBudget{}
	name := types.NamespacedName{Name: cluster.Name, Namespace: cluster.Namespace}

	// Compute the maximum number of replicas that may be unavailable while
	// preserving quorum: floor((replicas-1)/2).
	maxUnavailable := (cluster.Spec.Replicas - 1) / 2
	if maxUnavailable < 1 {
		maxUnavailable = 1
	}
	mu := intstr.FromInt(int(maxUnavailable))

	err := r.Get(ctx, name, pdb)
	if apierrors.IsNotFound(err) {
		pdb = &policyv1.PodDisruptionBudget{
			ObjectMeta: metav1.ObjectMeta{
				Name:      cluster.Name,
				Namespace: cluster.Namespace,
				Labels:    clusterLabels(cluster.Name),
			},
			Spec: policyv1.PodDisruptionBudgetSpec{
				MaxUnavailable: &mu,
				Selector: &metav1.LabelSelector{
					MatchLabels: clusterLabels(cluster.Name),
				},
			},
		}
		if err := controllerutil.SetControllerReference(cluster, pdb, r.Scheme); err != nil {
			return err
		}
		return r.Create(ctx, pdb)
	}
	if err != nil {
		return fmt.Errorf("get PodDisruptionBudget: %w", err)
	}

	pdb.Spec.MaxUnavailable = &mu
	return r.Update(ctx, pdb)
}

// ─────────────────────────────────────────────────────────────────────────────
// Replication ConfigMap
// ─────────────────────────────────────────────────────────────────────────────

// reconcileReplicationConfigMap keeps a ConfigMap that ThemisDB pods mount to
// get their replication topology configuration.
func (r *ThemisDBReconciler) reconcileReplicationConfigMap(ctx context.Context, cluster *vccv1alpha1.ThemisDB) error {
	cm := &corev1.ConfigMap{}
	name := types.NamespacedName{Name: replConfigMapName(cluster.Name), Namespace: cluster.Namespace}

	desired := buildReplicationConfigMap(cluster)
	err := r.Get(ctx, name, cm)
	if apierrors.IsNotFound(err) {
		if err := controllerutil.SetControllerReference(cluster, desired, r.Scheme); err != nil {
			return err
		}
		return r.Create(ctx, desired)
	}
	if err != nil {
		return fmt.Errorf("get replication configmap: %w", err)
	}

	cm.Data = desired.Data
	return r.Update(ctx, cm)
}

func buildReplicationConfigMap(cluster *vccv1alpha1.ThemisDB) *corev1.ConfigMap {
	repl := cluster.Spec.Replication
	data := map[string]string{
		"replication_mode":              string(repl.Mode),
		"min_sync_replicas":             fmt.Sprintf("%d", repl.MinSyncReplicas),
		"election_timeout_min_ms":       fmt.Sprintf("%d", repl.LeaderElection.ElectionTimeoutMinMs),
		"election_timeout_max_ms":       fmt.Sprintf("%d", repl.LeaderElection.ElectionTimeoutMaxMs),
		"heartbeat_interval_ms":         fmt.Sprintf("%d", repl.LeaderElection.HeartbeatIntervalMs),
		"leader_priority":               fmt.Sprintf("%d", repl.LeaderElection.LeaderPreferencePriority),
		"failover_enabled":              fmt.Sprintf("%v", repl.Failover.Enabled),
		"failure_detection_timeout_ms":  fmt.Sprintf("%d", repl.Failover.FailureDetectionTimeoutMs),
		"max_failover_attempts":         fmt.Sprintf("%d", repl.Failover.MaxFailoverAttempts),
		"failover_cooldown_ms":          fmt.Sprintf("%d", repl.Failover.FailoverCooldownMs),
		"degraded_lag_ms":               fmt.Sprintf("%d", repl.LagThresholds.DegradedLagMs),
		"critical_lag_ms":               fmt.Sprintf("%d", repl.LagThresholds.CriticalLagMs),
		"read_shift_enabled":            fmt.Sprintf("%v", repl.LagThresholds.ReadShiftEnabled),
		"wal_compression":               fmt.Sprintf("%v", repl.WAL.Compression),
		"wal_sync_on_commit":            fmt.Sprintf("%v", repl.WAL.SyncOnCommit),
		"wal_retention_hours":           fmt.Sprintf("%d", repl.WAL.RetentionHours),
		"topology_spread_across_zones":  fmt.Sprintf("%v", repl.TopologyAware.SpreadAcrossZones),
		"topology_spread_across_nodes":  fmt.Sprintf("%v", repl.TopologyAware.SpreadAcrossNodes),
	}

	return &corev1.ConfigMap{
		ObjectMeta: metav1.ObjectMeta{
			Name:      replConfigMapName(cluster.Name),
			Namespace: cluster.Namespace,
			Labels:    clusterLabels(cluster.Name),
		},
		Data: data,
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Topology Status
// ─────────────────────────────────────────────────────────────────────────────

// reconcileTopologyStatus inspects running pods, derives the current
// replication topology, and patches the ThemisDB status subresource.
// Returns (needsFastRequeue, error).
func (r *ThemisDBReconciler) reconcileTopologyStatus(ctx context.Context, cluster *vccv1alpha1.ThemisDB) (bool, error) {
	pods := &corev1.PodList{}
	if err := r.List(ctx, pods,
		client.InNamespace(cluster.Namespace),
		client.MatchingLabels(clusterLabels(cluster.Name)),
	); err != nil {
		return false, fmt.Errorf("list pods: %w", err)
	}

	topology, needsFastRequeue := deriveTopology(cluster, pods.Items)
	phase := derivePhase(cluster, pods.Items, topology)
	conditions := buildConditions(cluster, phase, topology)

	// Build the updated status.
	patch := cluster.DeepCopy()
	patch.Status.Phase = phase
	patch.Status.Replicas = int32(len(pods.Items))
	patch.Status.ReadyReplicas = countReady(pods.Items)
	patch.Status.ReplicationTopology = topology
	patch.Status.Conditions = conditions

	if err := r.Status().Update(ctx, patch); err != nil {
		return false, fmt.Errorf("update status: %w", err)
	}
	return needsFastRequeue, nil
}

// deriveTopology inspects pod annotations/labels and constructs the current
// replication topology status.  In production the operator queries the ThemisDB
// management API; here we derive it from pod ready conditions and annotations
// to remain self-contained.
func deriveTopology(cluster *vccv1alpha1.ThemisDB, pods []corev1.Pod) (vccv1alpha1.ReplicationTopologyStatus, bool) {
	topo := cluster.Status.ReplicationTopology

	var (
		inSync   []string
		lagging  []string
		replicas []vccv1alpha1.ReplicaHealthStatus
	)

	needsFastRequeue := false
	leaderFound := false

	for i := range pods {
		pod := &pods[i]
		nodeID := pod.Name
		role := replicaRoleFromAnnotation(pod)
		health := replicaHealthFromPod(pod)
		lagMs := lagMsFromAnnotation(pod)

		if role == vccv1alpha1.ReplicaRoleLeader {
			leaderFound = true
			topo.CurrentLeader = nodeID
		}

		if health == vccv1alpha1.ReplicaHealthHealthy || health == vccv1alpha1.ReplicaHealthDegraded {
			lagThreshold := cluster.Spec.Replication.LagThresholds.DegradedLagMs
			if lagThreshold == 0 {
				lagThreshold = 5000
			}
			criticalThreshold := cluster.Spec.Replication.LagThresholds.CriticalLagMs
			if criticalThreshold == 0 {
				criticalThreshold = 30000
			}

			if lagMs <= lagThreshold {
				inSync = append(inSync, nodeID)
			} else if lagMs <= criticalThreshold {
				lagging = append(lagging, nodeID)
			}
		}

		if health == vccv1alpha1.ReplicaHealthFailed || health == vccv1alpha1.ReplicaHealthDegraded {
			// A failed or degraded non-leader replica requires an expedited
			// health check so the operator can shift read traffic and trigger
			// failover if the leader is also affected.
			if role != vccv1alpha1.ReplicaRoleLeader {
				needsFastRequeue = true
			}
		}

		lastHeartbeat := metav1.NewTime(time.Now())
		replicas = append(replicas, vccv1alpha1.ReplicaHealthStatus{
			NodeID:        nodeID,
			Role:          role,
			Health:        health,
			LagMs:         lagMs,
			LastHeartbeat: &lastHeartbeat,
		})
	}

	// If no leader is annotated yet, keep the previous known leader.
	if !leaderFound && topo.CurrentLeader != "" {
		// Leader may have just changed; trigger fast requeue.
		needsFastRequeue = true
	}

	topo.InSyncReplicas = inSync
	topo.LaggingReplicas = lagging
	topo.ReplicaHealth = replicas
	return topo, needsFastRequeue
}

// replicaRoleFromAnnotation reads the pod annotation set by the ThemisDB
// process to advertise its current replication role.
func replicaRoleFromAnnotation(pod *corev1.Pod) vccv1alpha1.ReplicaRole {
	role, ok := pod.Annotations["vcc.io/replication-role"]
	if !ok {
		return vccv1alpha1.ReplicaRoleFollower
	}
	switch vccv1alpha1.ReplicaRole(role) {
	case vccv1alpha1.ReplicaRoleLeader:
		return vccv1alpha1.ReplicaRoleLeader
	case vccv1alpha1.ReplicaRoleFollower:
		return vccv1alpha1.ReplicaRoleFollower
	case vccv1alpha1.ReplicaRoleCandidate:
		return vccv1alpha1.ReplicaRoleCandidate
	case vccv1alpha1.ReplicaRoleObserver:
		return vccv1alpha1.ReplicaRoleObserver
	}
	return vccv1alpha1.ReplicaRoleFollower
}

// lagMsFromAnnotation reads the replication lag annotation published by the
// ThemisDB process.
func lagMsFromAnnotation(pod *corev1.Pod) int64 {
	val, ok := pod.Annotations["vcc.io/replication-lag-ms"]
	if !ok {
		return 0
	}
	var lag int64
	fmt.Sscanf(val, "%d", &lag)
	return lag
}

// replicaHealthFromPod derives the health state from the pod's Ready condition.
func replicaHealthFromPod(pod *corev1.Pod) vccv1alpha1.ReplicaHealth {
	if pod.DeletionTimestamp != nil {
		return vccv1alpha1.ReplicaHealthFailed
	}
	for _, cond := range pod.Status.Conditions {
		if cond.Type == corev1.PodReady {
			if cond.Status == corev1.ConditionTrue {
				return vccv1alpha1.ReplicaHealthHealthy
			}
			return vccv1alpha1.ReplicaHealthDegraded
		}
	}
	return vccv1alpha1.ReplicaHealthUnknown
}

func derivePhase(cluster *vccv1alpha1.ThemisDB, pods []corev1.Pod, topo vccv1alpha1.ReplicationTopologyStatus) vccv1alpha1.ClusterPhase {
	ready := countReady(pods)
	desired := cluster.Spec.Replicas

	if int32(len(pods)) == 0 {
		return vccv1alpha1.PhasePending
	}
	if ready == 0 {
		return vccv1alpha1.PhaseFailed
	}
	if ready < desired {
		if len(topo.LaggingReplicas) > 0 {
			return vccv1alpha1.PhaseDegraded
		}
		return vccv1alpha1.PhaseUpdating
	}
	return vccv1alpha1.PhaseRunning
}

func buildConditions(cluster *vccv1alpha1.ThemisDB, phase vccv1alpha1.ClusterPhase, topo vccv1alpha1.ReplicationTopologyStatus) []vccv1alpha1.ThemisDBCondition {
	now := metav1.Now()

	available := vccv1alpha1.ThemisDBCondition{
		Type:               vccv1alpha1.ConditionAvailable,
		LastTransitionTime: now,
	}
	topologyReady := vccv1alpha1.ThemisDBCondition{
		Type:               vccv1alpha1.ConditionTopologyReady,
		LastTransitionTime: now,
	}
	degraded := vccv1alpha1.ThemisDBCondition{
		Type:               vccv1alpha1.ConditionDegraded,
		LastTransitionTime: now,
	}

	if phase == vccv1alpha1.PhaseRunning {
		available.Status = "True"
		available.Reason = "MinReplicasAvailable"
		available.Message = fmt.Sprintf("All %d replicas are ready", cluster.Spec.Replicas)
	} else {
		available.Status = "False"
		available.Reason = "InsufficientReplicas"
		available.Message = fmt.Sprintf("Phase is %s", phase)
	}

	if topo.CurrentLeader != "" && len(topo.InSyncReplicas) >= int(cluster.Spec.Replication.MinSyncReplicas) {
		topologyReady.Status = "True"
		topologyReady.Reason = "QuorumAchieved"
		topologyReady.Message = fmt.Sprintf("Leader: %s, in-sync replicas: %d", topo.CurrentLeader, len(topo.InSyncReplicas))
	} else {
		topologyReady.Status = "False"
		topologyReady.Reason = "NoQuorum"
		topologyReady.Message = "Quorum not yet established"
	}

	if len(topo.LaggingReplicas) > 0 {
		degraded.Status = "True"
		degraded.Reason = "ReplicasLagging"
		degraded.Message = fmt.Sprintf("%d replica(s) exceeding lag threshold: %v", len(topo.LaggingReplicas), topo.LaggingReplicas)
	} else {
		degraded.Status = "False"
		degraded.Reason = "AllReplicasHealthy"
		degraded.Message = "No replicas are lagging"
	}

	return []vccv1alpha1.ThemisDBCondition{available, topologyReady, degraded}
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupWithManager
// ─────────────────────────────────────────────────────────────────────────────

// SetupWithManager registers the controller with the controller-runtime Manager.
func (r *ThemisDBReconciler) SetupWithManager(mgr ctrl.Manager) error {
	return ctrl.NewControllerManagedBy(mgr).
		For(&vccv1alpha1.ThemisDB{}).
		Owns(&appsv1.StatefulSet{}).
		Owns(&corev1.Service{}).
		Owns(&corev1.ConfigMap{}).
		Owns(&policyv1.PodDisruptionBudget{}).
		Complete(r)
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

func headlessSvcName(clusterName string) string {
	return clusterName + "-headless"
}

func replConfigMapName(clusterName string) string {
	return clusterName + "-replication-config"
}

func clusterLabels(clusterName string) map[string]string {
	return map[string]string{
		"app.kubernetes.io/name":       "themisdb",
		"app.kubernetes.io/instance":   clusterName,
		"app.kubernetes.io/managed-by": "themisdb-operator",
	}
}

func countReady(pods []corev1.Pod) int32 {
	var count int32
	for i := range pods {
		for _, cond := range pods[i].Status.Conditions {
			if cond.Type == corev1.PodReady && cond.Status == corev1.ConditionTrue {
				count++
				break
			}
		}
	}
	return count
}

// mustParseQuantity parses a resource quantity string; panics on invalid input
// (invalid values should be caught by CRD validation).
func mustParseQuantity(s string) resource.Quantity {
	return resource.MustParse(s)
}
