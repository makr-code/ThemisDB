import subprocess
import unittest
from pathlib import Path

import yaml


CHART = Path(__file__).resolve().parents[1]


def render(namespace, values_file=None):
    command = ["helm", "template", "themisdb", str(CHART), "--namespace", namespace]
    if values_file:
        command.extend(["--values", str(CHART / values_file)])
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    return [document for document in yaml.safe_load_all(result.stdout) if document]


def resources_by_kind(documents):
    return {document["kind"]: document for document in documents}


def selector_matches(selector, labels):
    return all(labels.get(key) == value for key, value in selector.get("matchLabels", {}).items())


def peer_matches(peer, namespace, labels, policy_namespace):
    has_namespace_selector = "namespaceSelector" in peer
    has_pod_selector = "podSelector" in peer
    namespace_selector = peer.get("namespaceSelector", {})
    pod_selector = peer.get("podSelector", {})
    if has_namespace_selector:
        namespace_labels = {"kubernetes.io/metadata.name": namespace}
        if not selector_matches(namespace_selector, namespace_labels):
            return False
    elif has_pod_selector and namespace != policy_namespace:
        return False
    if has_pod_selector and not selector_matches(pod_selector, labels):
        return False
    return has_namespace_selector or has_pod_selector


def rule_allows(rule, direction, namespace, labels, protocol, port, policy_namespace):
    ports = rule.get("ports")
    if ports and not any(
        entry.get("protocol", "TCP") == protocol and entry.get("port") == port
        for entry in ports
    ):
        return False

    peers = rule.get("from" if direction == "ingress" else "to")
    if peers is None:
        return True
    return any(peer_matches(peer, namespace, labels, policy_namespace) for peer in peers)


def policy_allows(policy, direction, namespace, labels, protocol, port, policy_namespace):
    if direction.capitalize() not in policy["spec"]["policyTypes"]:
        return True
    rules = policy["spec"].get(direction, [])
    return any(
        rule_allows(rule, direction, namespace, labels, protocol, port, policy_namespace)
        for rule in rules
    )


class RenderedProfileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.development = render("development")
        cls.production = render("production", "values-production.yaml")
        cls.dev_resources = resources_by_kind(cls.development)
        cls.prod_resources = resources_by_kind(cls.production)

    def assert_unique_resources(self, documents):
        identities = [
            (
                document["apiVersion"],
                document["kind"],
                document["metadata"].get("namespace", ""),
                document["metadata"]["name"],
            )
            for document in documents
        ]
        self.assertEqual(len(identities), len(set(identities)))

    def test_development_defaults_remain_explicit_and_usable(self):
        deployment = self.dev_resources["Deployment"]
        pod_spec = deployment["spec"]["template"]["spec"]
        self.assertEqual(deployment["spec"]["replicas"], 1)
        self.assertNotIn("PodDisruptionBudget", self.dev_resources)
        self.assertNotIn("NetworkPolicy", self.dev_resources)
        self.assertNotIn("affinity", pod_spec)
        self.assertNotIn("topologySpreadConstraints", pod_spec)
        self.assert_unique_resources(self.development)

    def test_production_profile_renders_ha_resources_and_selectors(self):
        deployment = self.prod_resources["Deployment"]
        pod_spec = deployment["spec"]["template"]["spec"]
        deployment_selector = deployment["spec"]["selector"]["matchLabels"]
        self.assertEqual(deployment["spec"]["replicas"], 3)

        pdb = self.prod_resources["PodDisruptionBudget"]
        self.assertEqual(pdb["spec"]["minAvailable"], 2)
        self.assertEqual(pdb["spec"]["selector"]["matchLabels"], deployment_selector)

        anti_affinity = pod_spec["affinity"]["podAntiAffinity"]
        preferred = anti_affinity["preferredDuringSchedulingIgnoredDuringExecution"]
        self.assertEqual(preferred[0]["podAffinityTerm"]["topologyKey"], "kubernetes.io/hostname")
        self.assertTrue(
            all(
                deployment_selector[key] == value
                for key, value in preferred[0]["podAffinityTerm"]["labelSelector"]["matchLabels"].items()
            )
        )

        spread = pod_spec["topologySpreadConstraints"]
        self.assertEqual(spread[0]["maxSkew"], 1)
        self.assertEqual(spread[0]["topologyKey"], "kubernetes.io/hostname")
        self.assertEqual(spread[0]["labelSelector"]["matchLabels"], deployment_selector)
        self.assert_unique_resources(self.production)

    def test_production_network_policy_selectors_health_and_default_deny(self):
        deployment = self.prod_resources["Deployment"]
        selector = deployment["spec"]["selector"]["matchLabels"]
        policy = self.prod_resources["NetworkPolicy"]
        self.assertEqual(policy["spec"]["podSelector"]["matchLabels"], selector)
        self.assertEqual(set(policy["spec"]["policyTypes"]), {"Ingress", "Egress"})

        probes = deployment["spec"]["template"]["spec"]["containers"][0]
        self.assertEqual(probes["readinessProbe"]["httpGet"]["port"], "health")
        self.assertEqual(probes["livenessProbe"]["httpGet"]["port"], "health")
        health_port = next(
            port["containerPort"]
            for port in deployment["spec"]["template"]["spec"]["containers"][0]["ports"]
            if port["name"] == "health"
        )
        self.assertTrue(
            policy_allows(policy, "ingress", "outside", {}, "TCP", health_port, "production")
        )

        self.assertTrue(
            policy_allows(policy, "ingress", "production", {"role": "client"}, "TCP", 8080, "production")
        )
        self.assertTrue(
            policy_allows(policy, "ingress", "monitoring", {}, "TCP", 9091, "production")
        )
        self.assertFalse(
            policy_allows(policy, "ingress", "outside", {}, "TCP", 8080, "production")
        )
        self.assertFalse(
            policy_allows(policy, "ingress", "monitoring", {}, "TCP", 8080, "production")
        )
        self.assertFalse(
            policy_allows(policy, "ingress", "production", {}, "TCP", 9999, "production")
        )

        dns_labels = {"k8s-app": "kube-dns"}
        self.assertTrue(
            policy_allows(policy, "egress", "kube-system", dns_labels, "UDP", 53, "production")
        )
        self.assertTrue(
            policy_allows(policy, "egress", "kube-system", dns_labels, "TCP", 53, "production")
        )
        self.assertFalse(
            policy_allows(policy, "egress", "outside", dns_labels, "UDP", 53, "production")
        )
        self.assertFalse(
            policy_allows(policy, "egress", "outside", {}, "TCP", 443, "production")
        )
        self.assertFalse(
            policy_allows(policy, "egress", "outside", {}, "TCP", 1234, "production")
        )

        self.assertTrue(
            policy_allows(policy, "egress", "production", selector, "TCP", 8766, "production")
        )
        self.assertTrue(
            policy_allows(policy, "egress", "production", selector, "UDP", 8769, "production")
        )
        self.assertFalse(
            policy_allows(policy, "egress", "outside", selector, "TCP", 8766, "production")
        )
        self.assertFalse(
            policy_allows(policy, "egress", "production", selector, "TCP", 8769, "production")
        )


if __name__ == "__main__":
    unittest.main()
