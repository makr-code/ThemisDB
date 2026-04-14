"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_ai_production_deployment.py                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1067                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Production Deployment Manager for Ethical AI Framework

Provides comprehensive container orchestration, health monitoring, and
deployment management for the ThemisDB Ethical AI Framework.

Features:
- Docker container lifecycle management
- Kubernetes orchestration with auto-scaling
- Health check monitoring with alerting
- Version management with rollback capabilities
- Load balancer configuration
- Production-ready security and resource management

Author: ThemisDB Team
License: MIT
"""

import os
import json
import time
import logging
import subprocess
import hashlib
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from enum import Enum
from pathlib import Path
import requests


logger = logging.getLogger(__name__)


class DeploymentStatus(Enum):
    """Deployment status enumeration."""
    PENDING = "pending"
    RUNNING = "running"
    HEALTHY = "healthy"
    UNHEALTHY = "unhealthy"
    FAILED = "failed"
    STOPPED = "stopped"
    ROLLING_BACK = "rolling_back"


class HealthStatus(Enum):
    """Health check status enumeration."""
    HEALTHY = "healthy"
    DEGRADED = "degraded"
    UNHEALTHY = "unhealthy"
    UNKNOWN = "unknown"


@dataclass
class ContainerConfig:
    """Configuration for Docker container deployment."""
    name: str
    image: str
    tag: str = "latest"
    ports: Dict[int, int] = field(default_factory=dict)
    environment: Dict[str, str] = field(default_factory=dict)
    volumes: Dict[str, Dict[str, str]] = field(default_factory=dict)
    command: Optional[List[str]] = None
    restart_policy: str = "unless-stopped"
    network: str = "ethics-ai-network"
    cpu_limit: str = "2.0"
    memory_limit: str = "2g"
    labels: Dict[str, str] = field(default_factory=dict)


@dataclass
class HealthCheckConfig:
    """Configuration for service health checks."""
    endpoint: str
    interval_seconds: int = 30
    timeout_seconds: int = 10
    retries: int = 3
    expected_status: int = 200
    method: str = "GET"
    headers: Dict[str, str] = field(default_factory=dict)
    body: Optional[str] = None


@dataclass
class DeploymentVersion:
    """Represents a deployment version."""
    version: str
    timestamp: datetime
    image_tag: str
    config_hash: str
    status: DeploymentStatus
    metadata: Dict[str, Any] = field(default_factory=dict)


class DockerContainerManager:
    """
    Manages Docker container lifecycle for ethical AI services.
    
    Provides:
    - Container creation and configuration
    - Lifecycle management (start, stop, restart)
    - Health monitoring integration
    - Log aggregation
    - Resource monitoring
    """
    
    def __init__(self, docker_socket: str = "/var/run/docker.sock"):
        """
        Initialize Docker container manager.
        
        Args:
            docker_socket: Path to Docker socket
        """
        self.docker_socket = docker_socket
        self.containers: Dict[str, ContainerConfig] = {}
        self._verify_docker()
        
    def _verify_docker(self) -> None:
        """Verify Docker daemon is accessible."""
        try:
            result = subprocess.run(
                ["docker", "info"],
                capture_output=True,
                text=True,
                check=True,
                timeout=10
            )
            logger.info("Docker daemon is accessible")
        except subprocess.CalledProcessError as e:
            logger.error(f"Docker daemon not accessible: {e}")
            raise RuntimeError("Docker daemon not running or not accessible")
        except FileNotFoundError:
            raise RuntimeError("Docker CLI not found. Please install Docker.")
    
    def create_network(self, network_name: str, driver: str = "bridge") -> bool:
        """
        Create Docker network for service communication.
        
        Args:
            network_name: Name of the network
            driver: Network driver (bridge, overlay, etc.)
            
        Returns:
            True if successful, False otherwise
        """
        try:
            # Check if network exists
            result = subprocess.run(
                ["docker", "network", "ls", "--filter", f"name={network_name}", "--format", "{{.Name}}"],
                capture_output=True,
                text=True,
                check=True
            )
            
            if network_name in result.stdout:
                logger.info(f"Network {network_name} already exists")
                return True
            
            # Create network
            subprocess.run(
                ["docker", "network", "create", "--driver", driver, network_name],
                capture_output=True,
                text=True,
                check=True
            )
            logger.info(f"Created Docker network: {network_name}")
            return True
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to create network {network_name}: {e}")
            return False
    
    def deploy_container(self, config: ContainerConfig) -> Tuple[bool, str]:
        """
        Deploy a Docker container with the given configuration.
        
        Args:
            config: Container configuration
            
        Returns:
            Tuple of (success, container_id_or_error)
        """
        try:
            # Build docker run command
            cmd = ["docker", "run", "-d"]
            
            # Name
            cmd.extend(["--name", config.name])
            
            # Network
            cmd.extend(["--network", config.network])
            
            # Resource limits
            cmd.extend(["--cpus", config.cpu_limit])
            cmd.extend(["--memory", config.memory_limit])
            
            # Restart policy
            cmd.extend(["--restart", config.restart_policy])
            
            # Ports
            for host_port, container_port in config.ports.items():
                cmd.extend(["-p", f"{host_port}:{container_port}"])
            
            # Environment variables
            for key, value in config.environment.items():
                cmd.extend(["-e", f"{key}={value}"])
            
            # Volumes
            for host_path, mount_config in config.volumes.items():
                container_path = mount_config.get("bind")
                mode = mount_config.get("mode", "rw")
                cmd.extend(["-v", f"{host_path}:{container_path}:{mode}"])
            
            # Labels
            for key, value in config.labels.items():
                cmd.extend(["--label", f"{key}={value}"])
            
            # Image
            cmd.append(f"{config.image}:{config.tag}")
            
            # Command
            if config.command:
                cmd.extend(config.command)
            
            # Execute
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True,
                timeout=60
            )
            
            container_id = result.stdout.strip()
            self.containers[config.name] = config
            logger.info(f"Deployed container {config.name}: {container_id}")
            
            return True, container_id
            
        except subprocess.CalledProcessError as e:
            error_msg = f"Failed to deploy container {config.name}: {e.stderr}"
            logger.error(error_msg)
            return False, error_msg
    
    def stop_container(self, name: str, timeout: int = 30) -> bool:
        """
        Stop a running container.
        
        Args:
            name: Container name
            timeout: Timeout in seconds before force kill
            
        Returns:
            True if successful
        """
        try:
            subprocess.run(
                ["docker", "stop", "-t", str(timeout), name],
                capture_output=True,
                text=True,
                check=True
            )
            logger.info(f"Stopped container: {name}")
            return True
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to stop container {name}: {e}")
            return False
    
    def remove_container(self, name: str, force: bool = False) -> bool:
        """
        Remove a container.
        
        Args:
            name: Container name
            force: Force removal of running container
            
        Returns:
            True if successful
        """
        try:
            cmd = ["docker", "rm"]
            if force:
                cmd.append("-f")
            cmd.append(name)
            
            subprocess.run(cmd, capture_output=True, text=True, check=True)
            
            if name in self.containers:
                del self.containers[name]
            
            logger.info(f"Removed container: {name}")
            return True
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to remove container {name}: {e}")
            return False
    
    def get_container_logs(self, name: str, tail: int = 100) -> Optional[str]:
        """
        Get container logs.
        
        Args:
            name: Container name
            tail: Number of lines to retrieve
            
        Returns:
            Log output or None
        """
        try:
            result = subprocess.run(
                ["docker", "logs", "--tail", str(tail), name],
                capture_output=True,
                text=True,
                check=True
            )
            return result.stdout
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to get logs for {name}: {e}")
            return None
    
    def get_container_stats(self, name: str) -> Optional[Dict[str, Any]]:
        """
        Get container resource statistics.
        
        Args:
            name: Container name
            
        Returns:
            Statistics dictionary or None
        """
        try:
            result = subprocess.run(
                ["docker", "stats", "--no-stream", "--format", "{{json .}}", name],
                capture_output=True,
                text=True,
                check=True
            )
            return json.loads(result.stdout)
            
        except (subprocess.CalledProcessError, json.JSONDecodeError) as e:
            logger.error(f"Failed to get stats for {name}: {e}")
            return None


class KubernetesOrchestrator:
    """
    Manages Kubernetes deployments for ethical AI services.
    
    Provides:
    - Deployment creation and updates
    - Service and ingress management
    - ConfigMap and Secret management
    - Auto-scaling configuration
    - Rolling updates and rollbacks
    """
    
    def __init__(self, kubeconfig: Optional[str] = None, namespace: str = "ethics-ai"):
        """
        Initialize Kubernetes orchestrator.
        
        Args:
            kubeconfig: Path to kubeconfig file (None for default)
            namespace: Kubernetes namespace
        """
        self.kubeconfig = kubeconfig or os.environ.get("KUBECONFIG", "~/.kube/config")
        self.namespace = namespace
        self._verify_kubectl()
        self._ensure_namespace()
    
    def _verify_kubectl(self) -> None:
        """Verify kubectl is accessible."""
        try:
            cmd = ["kubectl", "version", "--client", "--short"]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            
            subprocess.run(cmd, capture_output=True, text=True, check=True, timeout=10)
            logger.info("kubectl is accessible")
            
        except subprocess.CalledProcessError as e:
            logger.error(f"kubectl not accessible: {e}")
            raise RuntimeError("kubectl not found or not configured")
        except FileNotFoundError:
            raise RuntimeError("kubectl not found. Please install kubectl.")
    
    def _ensure_namespace(self) -> None:
        """Ensure namespace exists."""
        try:
            cmd = ["kubectl", "get", "namespace", self.namespace]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                # Create namespace
                create_cmd = ["kubectl", "create", "namespace", self.namespace]
                if self.kubeconfig:
                    create_cmd.extend(["--kubeconfig", self.kubeconfig])
                
                subprocess.run(create_cmd, capture_output=True, text=True, check=True)
                logger.info(f"Created namespace: {self.namespace}")
            else:
                logger.info(f"Namespace {self.namespace} exists")
                
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to ensure namespace: {e}")
    
    def apply_manifest(self, manifest_path: str) -> Tuple[bool, str]:
        """
        Apply a Kubernetes manifest file.
        
        Args:
            manifest_path: Path to manifest YAML file
            
        Returns:
            Tuple of (success, output_or_error)
        """
        try:
            cmd = ["kubectl", "apply", "-f", manifest_path, "-n", self.namespace]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True
            )
            
            logger.info(f"Applied manifest: {manifest_path}")
            return True, result.stdout
            
        except subprocess.CalledProcessError as e:
            error_msg = f"Failed to apply manifest {manifest_path}: {e.stderr}"
            logger.error(error_msg)
            return False, error_msg
    
    def delete_manifest(self, manifest_path: str) -> Tuple[bool, str]:
        """
        Delete resources from a Kubernetes manifest file.
        
        Args:
            manifest_path: Path to manifest YAML file
            
        Returns:
            Tuple of (success, output_or_error)
        """
        try:
            cmd = ["kubectl", "delete", "-f", manifest_path, "-n", self.namespace]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True
            )
            
            logger.info(f"Deleted manifest: {manifest_path}")
            return True, result.stdout
            
        except subprocess.CalledProcessError as e:
            error_msg = f"Failed to delete manifest {manifest_path}: {e.stderr}"
            logger.error(error_msg)
            return False, error_msg
    
    def get_deployment_status(self, deployment_name: str) -> Optional[Dict[str, Any]]:
        """
        Get deployment status.
        
        Args:
            deployment_name: Name of deployment
            
        Returns:
            Status dictionary or None
        """
        try:
            cmd = [
                "kubectl", "get", "deployment", deployment_name,
                "-n", self.namespace,
                "-o", "json"
            ]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True
            )
            
            return json.loads(result.stdout)
            
        except (subprocess.CalledProcessError, json.JSONDecodeError) as e:
            logger.error(f"Failed to get deployment status for {deployment_name}: {e}")
            return None
    
    def scale_deployment(self, deployment_name: str, replicas: int) -> bool:
        """
        Scale a deployment.
        
        Args:
            deployment_name: Name of deployment
            replicas: Number of replicas
            
        Returns:
            True if successful
        """
        try:
            cmd = [
                "kubectl", "scale", "deployment", deployment_name,
                f"--replicas={replicas}",
                "-n", self.namespace
            ]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            
            subprocess.run(cmd, capture_output=True, text=True, check=True)
            
            logger.info(f"Scaled deployment {deployment_name} to {replicas} replicas")
            return True
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to scale deployment {deployment_name}: {e}")
            return False
    
    def rollout_restart(self, deployment_name: str) -> bool:
        """
        Restart a deployment (rolling restart).
        
        Args:
            deployment_name: Name of deployment
            
        Returns:
            True if successful
        """
        try:
            cmd = [
                "kubectl", "rollout", "restart", "deployment", deployment_name,
                "-n", self.namespace
            ]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            
            subprocess.run(cmd, capture_output=True, text=True, check=True)
            
            logger.info(f"Restarted deployment: {deployment_name}")
            return True
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to restart deployment {deployment_name}: {e}")
            return False
    
    def rollout_undo(self, deployment_name: str, revision: Optional[int] = None) -> bool:
        """
        Rollback a deployment to previous revision.
        
        Args:
            deployment_name: Name of deployment
            revision: Specific revision to rollback to (None for previous)
            
        Returns:
            True if successful
        """
        try:
            cmd = ["kubectl", "rollout", "undo", "deployment", deployment_name, "-n", self.namespace]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            if revision:
                cmd.append(f"--to-revision={revision}")
            
            subprocess.run(cmd, capture_output=True, text=True, check=True)
            
            logger.info(f"Rolled back deployment {deployment_name}")
            return True
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to rollback deployment {deployment_name}: {e}")
            return False
    
    def get_pod_logs(self, pod_name: str, container: Optional[str] = None, tail: int = 100) -> Optional[str]:
        """
        Get pod logs.
        
        Args:
            pod_name: Name of pod
            container: Container name (for multi-container pods)
            tail: Number of lines to retrieve
            
        Returns:
            Log output or None
        """
        try:
            cmd = ["kubectl", "logs", pod_name, "-n", self.namespace, f"--tail={tail}"]
            if self.kubeconfig:
                cmd.extend(["--kubeconfig", self.kubeconfig])
            if container:
                cmd.extend(["-c", container])
            
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            return result.stdout
            
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to get logs for pod {pod_name}: {e}")
            return None


class HealthCheckManager:
    """
    Manages health checks for deployed services.
    
    Provides:
    - HTTP/TCP health probes
    - Continuous monitoring
    - Alert generation
    - Health history tracking
    - Automatic remediation triggers
    """
    
    def __init__(self):
        """Initialize health check manager."""
        self.health_checks: Dict[str, HealthCheckConfig] = {}
        self.health_history: Dict[str, List[Tuple[datetime, HealthStatus]]] = {}
        self.alert_callbacks: List[callable] = []
    
    def register_health_check(self, service_name: str, config: HealthCheckConfig) -> None:
        """
        Register a health check for a service.
        
        Args:
            service_name: Name of the service
            config: Health check configuration
        """
        self.health_checks[service_name] = config
        self.health_history[service_name] = []
        logger.info(f"Registered health check for {service_name}")
    
    def add_alert_callback(self, callback: callable) -> None:
        """
        Add a callback for health alerts.
        
        Args:
            callback: Function to call on health state change
                      Signature: callback(service_name, old_status, new_status)
        """
        self.alert_callbacks.append(callback)
    
    def check_health(self, service_name: str) -> Tuple[HealthStatus, str]:
        """
        Perform a health check on a service.
        
        Args:
            service_name: Name of the service
            
        Returns:
            Tuple of (status, message)
        """
        if service_name not in self.health_checks:
            return HealthStatus.UNKNOWN, "Health check not configured"
        
        config = self.health_checks[service_name]
        
        try:
            # Perform HTTP health check
            if config.method.upper() == "GET":
                response = requests.get(
                    config.endpoint,
                    headers=config.headers,
                    timeout=config.timeout_seconds
                )
            elif config.method.upper() == "POST":
                response = requests.post(
                    config.endpoint,
                    headers=config.headers,
                    data=config.body,
                    timeout=config.timeout_seconds
                )
            else:
                return HealthStatus.UNKNOWN, f"Unsupported method: {config.method}"
            
            # Check response
            if response.status_code == config.expected_status:
                status = HealthStatus.HEALTHY
                message = "Service is healthy"
            else:
                status = HealthStatus.DEGRADED
                message = f"Unexpected status code: {response.status_code}"
            
            # Record history
            self._record_health_status(service_name, status)
            
            return status, message
            
        except requests.exceptions.Timeout:
            status = HealthStatus.UNHEALTHY
            message = "Health check timed out"
            self._record_health_status(service_name, status)
            return status, message
            
        except requests.exceptions.ConnectionError:
            status = HealthStatus.UNHEALTHY
            message = "Cannot connect to service"
            self._record_health_status(service_name, status)
            return status, message
            
        except Exception as e:
            status = HealthStatus.UNKNOWN
            message = f"Health check error: {str(e)}"
            self._record_health_status(service_name, status)
            return status, message
    
    def _record_health_status(self, service_name: str, status: HealthStatus) -> None:
        """Record health status in history."""
        timestamp = datetime.now()
        
        # Get previous status
        history = self.health_history[service_name]
        previous_status = history[-1][1] if history else None
        
        # Record new status
        history.append((timestamp, status))
        
        # Trim history (keep last 100 entries)
        if len(history) > 100:
            self.health_history[service_name] = history[-100:]
        
        # Trigger alerts if status changed
        if previous_status and previous_status != status:
            self._trigger_alerts(service_name, previous_status, status)
    
    def _trigger_alerts(self, service_name: str, old_status: HealthStatus, new_status: HealthStatus) -> None:
        """Trigger alert callbacks."""
        for callback in self.alert_callbacks:
            try:
                callback(service_name, old_status, new_status)
            except Exception as e:
                logger.error(f"Alert callback failed: {e}")
    
    def monitor_continuously(self, interval_seconds: int = 30, duration_seconds: Optional[int] = None) -> None:
        """
        Continuously monitor all registered services.
        
        Args:
            interval_seconds: Check interval
            duration_seconds: Total duration (None for infinite)
        """
        start_time = time.time()
        
        while True:
            # Check all services
            for service_name in self.health_checks:
                status, message = self.check_health(service_name)
                logger.info(f"Health check {service_name}: {status.value} - {message}")
            
            # Check duration
            if duration_seconds and (time.time() - start_time) >= duration_seconds:
                break
            
            # Wait for next interval
            time.sleep(interval_seconds)


class VersionManager:
    """
    Manages deployment versioning and rollback.
    
    Provides:
    - Version tracking
    - Deployment history
    - Rollback capabilities
    - Version comparison
    - Configuration checksums
    """
    
    def __init__(self, state_file: str = "/var/lib/ethics-ai/versions.json"):
        """
        Initialize version manager.
        
        Args:
            state_file: Path to state file
        """
        self.state_file = Path(state_file)
        self.state_file.parent.mkdir(parents=True, exist_ok=True)
        self.versions: List[DeploymentVersion] = []
        self._load_state()
    
    def _load_state(self) -> None:
        """Load version state from file."""
        if self.state_file.exists():
            try:
                with open(self.state_file, 'r') as f:
                    data = json.load(f)
                    self.versions = [
                        DeploymentVersion(
                            version=v["version"],
                            timestamp=datetime.fromisoformat(v["timestamp"]),
                            image_tag=v["image_tag"],
                            config_hash=v["config_hash"],
                            status=DeploymentStatus(v["status"]),
                            metadata=v.get("metadata", {})
                        )
                        for v in data
                    ]
                logger.info(f"Loaded {len(self.versions)} versions from state")
            except Exception as e:
                logger.error(f"Failed to load version state: {e}")
    
    def _save_state(self) -> None:
        """Save version state to file."""
        try:
            data = [
                {
                    "version": v.version,
                    "timestamp": v.timestamp.isoformat(),
                    "image_tag": v.image_tag,
                    "config_hash": v.config_hash,
                    "status": v.status.value,
                    "metadata": v.metadata
                }
                for v in self.versions
            ]
            
            with open(self.state_file, 'w') as f:
                json.dump(data, f, indent=2)
                
            logger.info("Saved version state")
            
        except Exception as e:
            logger.error(f"Failed to save version state: {e}")
    
    def register_deployment(
        self,
        version: str,
        image_tag: str,
        config: Dict[str, Any],
        metadata: Optional[Dict[str, Any]] = None
    ) -> DeploymentVersion:
        """
        Register a new deployment version.
        
        Args:
            version: Version identifier
            image_tag: Container image tag
            config: Deployment configuration
            metadata: Additional metadata
            
        Returns:
            DeploymentVersion object
        """
        # Calculate config hash
        config_str = json.dumps(config, sort_keys=True)
        config_hash = hashlib.sha256(config_str.encode()).hexdigest()[:16]
        
        # Create version
        deployment_version = DeploymentVersion(
            version=version,
            timestamp=datetime.now(),
            image_tag=image_tag,
            config_hash=config_hash,
            status=DeploymentStatus.PENDING,
            metadata=metadata or {}
        )
        
        self.versions.append(deployment_version)
        self._save_state()
        
        logger.info(f"Registered deployment version: {version}")
        return deployment_version
    
    def update_status(self, version: str, status: DeploymentStatus) -> bool:
        """
        Update deployment status.
        
        Args:
            version: Version identifier
            status: New status
            
        Returns:
            True if successful
        """
        for v in self.versions:
            if v.version == version:
                v.status = status
                self._save_state()
                logger.info(f"Updated version {version} status to {status.value}")
                return True
        
        logger.warning(f"Version {version} not found")
        return False
    
    def get_current_version(self) -> Optional[DeploymentVersion]:
        """
        Get the current active deployment version.
        
        Returns:
            Current DeploymentVersion or None
        """
        for v in reversed(self.versions):
            if v.status in [DeploymentStatus.RUNNING, DeploymentStatus.HEALTHY]:
                return v
        return None
    
    def get_previous_version(self, exclude_failed: bool = True) -> Optional[DeploymentVersion]:
        """
        Get the previous deployment version for rollback.
        
        Args:
            exclude_failed: Exclude failed deployments
            
        Returns:
            Previous DeploymentVersion or None
        """
        current = self.get_current_version()
        if not current:
            return None
        
        for v in reversed(self.versions):
            if v.version != current.version:
                if exclude_failed and v.status == DeploymentStatus.FAILED:
                    continue
                return v
        
        return None
    
    def list_versions(self, limit: int = 10) -> List[DeploymentVersion]:
        """
        List recent deployment versions.
        
        Args:
            limit: Maximum number of versions to return
            
        Returns:
            List of DeploymentVersion objects
        """
        return list(reversed(self.versions[-limit:]))


def create_ethics_ai_stack() -> Dict[str, ContainerConfig]:
    """
    Create complete Ethics AI stack configuration.
    
    Returns:
        Dictionary of container configurations
    """
    stack = {}
    
    # ThemisDB
    stack["themisdb"] = ContainerConfig(
        name="ethics-themisdb",
        image="themisdb/themisdb",
        tag="latest",
        ports={8529: 8529, 9090: 9090},
        environment={
            "THEMIS_ENABLE_LLM": "true",
            "THEMIS_HTTP_PORT": "8529",
            "THEMIS_METRICS_PORT": "9090",
            "THEMIS_LOG_LEVEL": "info"
        },
        volumes={
            "/var/lib/ethics-ai/themisdb/data": {"bind": "/var/lib/themisdb/data", "mode": "rw"},
            "/var/lib/ethics-ai/themisdb/logs": {"bind": "/var/lib/themisdb/logs", "mode": "rw"}
        },
        cpu_limit="4.0",
        memory_limit="8g",
        labels={"service": "database", "component": "ethics-ai"}
    )
    
    # Ethics AI Service
    stack["ethics-ai"] = ContainerConfig(
        name="ethics-ai-service",
        image="themisdb/ethics-ai",
        tag="latest",
        ports={8080: 8080, 8081: 8081},
        environment={
            "THEMIS_HOST": "ethics-themisdb",
            "THEMIS_PORT": "8529",
            "SERVICE_PORT": "8080",
            "METRICS_PORT": "8081",
            "LOG_LEVEL": "info"
        },
        cpu_limit="2.0",
        memory_limit="4g",
        labels={"service": "ethics-ai", "component": "application"}
    )
    
    # Prometheus
    stack["prometheus"] = ContainerConfig(
        name="ethics-prometheus",
        image="prom/prometheus",
        tag="latest",
        ports={9091: 9090},
        volumes={
            "/var/lib/ethics-ai/prometheus": {"bind": "/etc/prometheus", "mode": "ro"}
        },
        cpu_limit="1.0",
        memory_limit="2g",
        labels={"service": "monitoring", "component": "metrics"}
    )
    
    # Grafana
    stack["grafana"] = ContainerConfig(
        name="ethics-grafana",
        image="grafana/grafana",
        tag="latest",
        ports={3000: 3000},
        environment={
            "GF_SECURITY_ADMIN_PASSWORD": os.environ.get("GRAFANA_ADMIN_PASSWORD", "CHANGE-ME-IN-PRODUCTION"),
            "GF_INSTALL_PLUGINS": "grafana-piechart-panel"
        },
        volumes={
            "/var/lib/ethics-ai/grafana": {"bind": "/var/lib/grafana", "mode": "rw"}
        },
        cpu_limit="0.5",
        memory_limit="1g",
        labels={"service": "monitoring", "component": "visualization"}
    )
    
    return stack


if __name__ == "__main__":
    # Configure logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    # Example usage
    logger.info("Ethics AI Production Deployment Manager")
    logger.info("=" * 60)
    
    # Initialize managers
    docker_mgr = DockerContainerManager()
    k8s_mgr = KubernetesOrchestrator(namespace="ethics-ai")
    health_mgr = HealthCheckManager()
    version_mgr = VersionManager()
    
    logger.info("Initialized all managers successfully")
