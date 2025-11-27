"""Backup and restore utilities for vector stores."""

import asyncio
import json
import shutil
import tarfile
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Optional


@dataclass
class BackupConfig:
    """Configuration for backup operations."""
    
    backup_dir: str = "./backups"
    compress: bool = True
    include_metadata: bool = True
    retention_days: int = 7


@dataclass
class BackupMetadata:
    """Metadata for a backup."""
    
    created_at: str
    vector_store_type: str
    document_count: int
    size_bytes: int
    version: str = "1.0"
    extra: dict = field(default_factory=dict)


class VectorStoreBackup:
    """Backup and restore for vector stores."""
    
    def __init__(self, vector_store, config: Optional[BackupConfig] = None):
        """Initialize backup handler.
        
        Args:
            vector_store: The vector store instance to backup.
            config: Backup configuration.
        """
        self.store = vector_store
        self.config = config or BackupConfig()
        Path(self.config.backup_dir).mkdir(parents=True, exist_ok=True)
    
    async def create(self, output_path: Optional[str] = None) -> str:
        """Create a backup of the vector store.
        
        Args:
            output_path: Custom output path. If None, auto-generated.
            
        Returns:
            Path to the created backup file.
        """
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        if output_path is None:
            filename = f"respo_backup_{timestamp}"
            if self.config.compress:
                filename += ".tar.gz"
            output_path = str(Path(self.config.backup_dir) / filename)
        
        # Get store type
        store_type = type(self.store).__name__
        
        # Create temp directory for backup
        temp_dir = Path(self.config.backup_dir) / f"temp_{timestamp}"
        temp_dir.mkdir(parents=True, exist_ok=True)
        
        try:
            # Export data based on store type
            if hasattr(self.store, 'export_data'):
                data = await self.store.export_data()
            elif hasattr(self.store, 'persist_directory'):
                # ChromaDB - copy persist directory
                src = Path(self.store.persist_directory)
                if src.exists():
                    shutil.copytree(src, temp_dir / "data")
                data = None
            else:
                # Generic: try to get all documents
                data = {"documents": [], "embeddings": [], "metadata": []}
            
            # Save exported data if available
            if data is not None:
                with open(temp_dir / "data.json", "w") as f:
                    json.dump(data, f)
            
            # Create metadata
            if self.config.include_metadata:
                doc_count = 0
                if hasattr(self.store, 'count'):
                    doc_count = await self.store.count()
                
                metadata = BackupMetadata(
                    created_at=datetime.now().isoformat(),
                    vector_store_type=store_type,
                    document_count=doc_count,
                    size_bytes=sum(f.stat().st_size for f in temp_dir.rglob("*") if f.is_file()),
                )
                
                with open(temp_dir / "metadata.json", "w") as f:
                    json.dump(metadata.__dict__, f, indent=2)
            
            # Compress if configured
            if self.config.compress:
                with tarfile.open(output_path, "w:gz") as tar:
                    tar.add(temp_dir, arcname="backup")
            else:
                shutil.move(str(temp_dir), output_path)
            
            return output_path
            
        finally:
            # Cleanup temp directory
            if temp_dir.exists():
                shutil.rmtree(temp_dir)
    
    async def restore(self, backup_path: str) -> bool:
        """Restore vector store from backup.
        
        Args:
            backup_path: Path to the backup file.
            
        Returns:
            True if restore was successful.
        """
        backup_path = Path(backup_path)
        
        if not backup_path.exists():
            raise FileNotFoundError(f"Backup not found: {backup_path}")
        
        # Create temp directory for extraction
        temp_dir = Path(self.config.backup_dir) / f"restore_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        temp_dir.mkdir(parents=True, exist_ok=True)
        
        try:
            # Extract if compressed
            if str(backup_path).endswith(".tar.gz"):
                with tarfile.open(backup_path, "r:gz") as tar:
                    tar.extractall(temp_dir)
                extract_dir = temp_dir / "backup"
            else:
                extract_dir = backup_path
            
            # Load and verify metadata
            metadata_path = extract_dir / "metadata.json"
            if metadata_path.exists():
                with open(metadata_path) as f:
                    metadata = json.load(f)
                print(f"Restoring backup from {metadata.get('created_at', 'unknown')}")
                print(f"Documents: {metadata.get('document_count', 'unknown')}")
            
            # Restore data
            if hasattr(self.store, 'import_data'):
                data_path = extract_dir / "data.json"
                if data_path.exists():
                    with open(data_path) as f:
                        data = json.load(f)
                    await self.store.import_data(data)
            elif hasattr(self.store, 'persist_directory'):
                # ChromaDB - copy to persist directory
                src = extract_dir / "data"
                if src.exists():
                    dst = Path(self.store.persist_directory)
                    if dst.exists():
                        shutil.rmtree(dst)
                    shutil.copytree(src, dst)
            
            return True
            
        finally:
            # Cleanup
            if temp_dir.exists():
                shutil.rmtree(temp_dir)
    
    async def list_backups(self) -> list[dict]:
        """List available backups.
        
        Returns:
            List of backup metadata dictionaries.
        """
        backups = []
        backup_dir = Path(self.config.backup_dir)
        
        for path in backup_dir.glob("respo_backup_*"):
            info = {
                "path": str(path),
                "filename": path.name,
                "size_bytes": path.stat().st_size if path.is_file() else 0,
                "created": datetime.fromtimestamp(path.stat().st_mtime).isoformat(),
            }
            backups.append(info)
        
        return sorted(backups, key=lambda x: x["created"], reverse=True)
    
    async def cleanup_old_backups(self) -> int:
        """Remove backups older than retention period.
        
        Returns:
            Number of backups removed.
        """
        from datetime import timedelta
        
        cutoff = datetime.now() - timedelta(days=self.config.retention_days)
        removed = 0
        
        for backup in await self.list_backups():
            created = datetime.fromisoformat(backup["created"])
            if created < cutoff:
                path = Path(backup["path"])
                if path.is_file():
                    path.unlink()
                else:
                    shutil.rmtree(path)
                removed += 1
        
        return removed


class ScheduledBackup:
    """Scheduled backup runner."""
    
    def __init__(
        self,
        vector_store,
        backup_dir: str = "./backups",
        interval_hours: int = 24,
        retention_days: int = 7,
    ):
        """Initialize scheduled backup.
        
        Args:
            vector_store: Vector store to backup.
            backup_dir: Directory to store backups.
            interval_hours: Hours between backups.
            retention_days: Days to keep backups.
        """
        config = BackupConfig(
            backup_dir=backup_dir,
            retention_days=retention_days,
        )
        self.backup = VectorStoreBackup(vector_store, config)
        self.interval_hours = interval_hours
        self._running = False
        self._task: Optional[asyncio.Task] = None
    
    async def start(self):
        """Start scheduled backup loop."""
        self._running = True
        self._task = asyncio.create_task(self._run_loop())
    
    async def stop(self):
        """Stop scheduled backup loop."""
        self._running = False
        if self._task:
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass
    
    async def _run_loop(self):
        """Background backup loop."""
        while self._running:
            try:
                # Create backup
                path = await self.backup.create()
                print(f"Scheduled backup created: {path}")
                
                # Cleanup old backups
                removed = await self.backup.cleanup_old_backups()
                if removed > 0:
                    print(f"Removed {removed} old backups")
                
            except Exception as e:
                print(f"Backup error: {e}")
            
            # Wait for next interval
            await asyncio.sleep(self.interval_hours * 3600)
