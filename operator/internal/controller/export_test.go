/*
 * ThemisDB Kubernetes Operator – Exported helpers for testing
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

package controller

import (
	appsv1 "k8s.io/api/apps/v1"
	corev1 "k8s.io/api/core/v1"

	vccv1alpha1 "github.com/makr-code/ThemisDB/operator/api/v1alpha1"
)

// The following functions are thin wrappers that expose the package-internal
// helpers to the external test package (_test suffix).

// CountReady exposes countReady.
func CountReady(pods []corev1.Pod) int32 { return countReady(pods) }

// ClusterLabels exposes clusterLabels.
func ClusterLabels(name string) map[string]string { return clusterLabels(name) }

// HeadlessSvcName exposes headlessSvcName.
func HeadlessSvcName(name string) string { return headlessSvcName(name) }

// ReplConfigMapName exposes replConfigMapName.
func ReplConfigMapName(name string) string { return replConfigMapName(name) }

// DerivePhase exposes derivePhase.
func DerivePhase(cluster *vccv1alpha1.ThemisDB, pods []corev1.Pod, topo vccv1alpha1.ReplicationTopologyStatus) vccv1alpha1.ClusterPhase {
	return derivePhase(cluster, pods, topo)
}

// DeriveTopology exposes deriveTopology.
func DeriveTopology(cluster *vccv1alpha1.ThemisDB, pods []corev1.Pod) (vccv1alpha1.ReplicationTopologyStatus, bool) {
	return deriveTopology(cluster, pods)
}

// BuildConditions exposes buildConditions.
func BuildConditions(cluster *vccv1alpha1.ThemisDB, phase vccv1alpha1.ClusterPhase, topo vccv1alpha1.ReplicationTopologyStatus) []vccv1alpha1.ThemisDBCondition {
	return buildConditions(cluster, phase, topo)
}

// BuildReplicationConfigMap exposes buildReplicationConfigMap.
func BuildReplicationConfigMap(cluster *vccv1alpha1.ThemisDB) *corev1.ConfigMap {
	return buildReplicationConfigMap(cluster)
}

// BuildHeadlessService exposes buildHeadlessService.
func BuildHeadlessService(cluster *vccv1alpha1.ThemisDB) *corev1.Service {
	return buildHeadlessService(cluster)
}

// BuildStatefulSet exposes buildStatefulSet on the reconciler.
func (r *ThemisDBReconciler) BuildStatefulSet(cluster *vccv1alpha1.ThemisDB) *appsv1.StatefulSet {
	return r.buildStatefulSet(cluster)
}

// ReplicaHealthFromPod exposes replicaHealthFromPod.
func ReplicaHealthFromPod(pod *corev1.Pod) vccv1alpha1.ReplicaHealth {
	return replicaHealthFromPod(pod)
}

// ReplicaRoleFromAnnotation exposes replicaRoleFromAnnotation.
func ReplicaRoleFromAnnotation(pod *corev1.Pod) vccv1alpha1.ReplicaRole {
	return replicaRoleFromAnnotation(pod)
}

// LagMsFromAnnotation exposes lagMsFromAnnotation.
func LagMsFromAnnotation(pod *corev1.Pod) int64 {
	return lagMsFromAnnotation(pod)
}
