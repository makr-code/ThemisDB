#!/usr/bin/env bash
set -euo pipefail

for command in docker kind helm kubectl; do
  if ! command -v "$command" >/dev/null 2>&1; then
    printf 'Required command not found: %s\n' "$command" >&2
    exit 1
  fi
done

docker info >/dev/null

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/../../.." && pwd)"
chart_dir="$repo_root/helm/themisdb"
cluster_name="themisdb-helm-smoke-$(id -u)-$$"
work_dir="$(mktemp -d)"
kubeconfig="$work_dir/kubeconfig"
dev_namespace="helm-smoke-dev-$$"
prod_namespace="helm-smoke-prod-$$"

cleanup() {
  kind delete cluster --name "$cluster_name" --kubeconfig "$kubeconfig" >/dev/null 2>&1 || true
  rm -rf "$work_dir"
}
trap cleanup EXIT

export KUBECONFIG="$kubeconfig"

assert_equal() {
  local expected="$1"
  local actual="$2"
  local description="$3"

  if [[ "$actual" != "$expected" ]]; then
    printf '%s: expected "%s", got "%s"\n' "$description" "$expected" "$actual" >&2
    exit 1
  fi
}

kind create cluster --name "$cluster_name" --kubeconfig "$kubeconfig" --wait 120s
kubectl create namespace "$dev_namespace"
kubectl create namespace "$prod_namespace"

helm template development "$chart_dir" --namespace "$dev_namespace" |
  kubectl --namespace "$dev_namespace" apply --dry-run=server -f -
helm template production "$chart_dir" --namespace "$prod_namespace" \
  --values "$chart_dir/values-production.yaml" |
  kubectl --namespace "$prod_namespace" apply --dry-run=server -f -

helm install development "$chart_dir" --namespace "$dev_namespace" \
  --set persistence.enabled=false
helm install production "$chart_dir" --namespace "$prod_namespace" \
  --values "$chart_dir/values-production.yaml" \
  --set persistence.enabled=false

dev_deployment="development-themisdb"
prod_deployment="production-themisdb"

assert_equal "1" \
  "$(kubectl get deployment "$dev_deployment" --namespace "$dev_namespace" -o jsonpath='{.spec.replicas}')" \
  "development replica count"
assert_equal "3" \
  "$(kubectl get deployment "$prod_deployment" --namespace "$prod_namespace" -o jsonpath='{.spec.replicas}')" \
  "production replica count"
assert_equal "2" \
  "$(kubectl get pdb "$prod_deployment" --namespace "$prod_namespace" -o jsonpath='{.spec.minAvailable}')" \
  "production PDB minAvailable"
assert_equal "kubernetes.io/hostname" \
  "$(kubectl get deployment "$prod_deployment" --namespace "$prod_namespace" -o jsonpath='{.spec.template.spec.topologySpreadConstraints[0].topologyKey}')" \
  "production topology spread key"
assert_equal "100" \
  "$(kubectl get deployment "$prod_deployment" --namespace "$prod_namespace" -o jsonpath='{.spec.template.spec.affinity.podAntiAffinity.preferredDuringSchedulingIgnoredDuringExecution[0].weight}')" \
  "production anti-affinity weight"
assert_equal "Ingress Egress" \
  "$(kubectl get networkpolicy "$prod_deployment" --namespace "$prod_namespace" -o jsonpath='{.spec.policyTypes[0]} {.spec.policyTypes[1]}')" \
  "production NetworkPolicy types"

if kubectl get pdb "$dev_deployment" --namespace "$dev_namespace" >/dev/null 2>&1; then
  printf 'Development profile unexpectedly rendered a PodDisruptionBudget\n' >&2
  exit 1
fi
if kubectl get networkpolicy "$dev_deployment" --namespace "$dev_namespace" >/dev/null 2>&1; then
  printf 'Development profile unexpectedly rendered a NetworkPolicy\n' >&2
  exit 1
fi

printf 'Kind smoke test passed for development and production Helm profiles.\n'
