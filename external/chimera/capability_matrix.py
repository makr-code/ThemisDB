"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            capability_matrix.py                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:04:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     301                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8964bef5bc  2026-02-28  feat(chimera): implement CapabilityMatrix for adapter cap... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
CHIMERA Suite: Adapter Capability Matrix

Provides a vendor-neutral cross-reference matrix that shows which
capabilities (operations) each registered database adapter supports.

This module satisfies ROADMAP Issue #2376:
  "Adapter capability matrix (which operations each system supports)"

Standards:
    - IEEE Std 2807-2022: Framework for Artificial Intelligence (AI) in Databases
    - ISO/IEC 14756:2015: Performance benchmarking
"""

from typing import Any, Dict, Iterable, List, Optional, Protocol, runtime_checkable


# ---------------------------------------------------------------------------
# Capability protocol — any object that exposes get_capabilities() qualifies
# ---------------------------------------------------------------------------

@runtime_checkable
class CapableAdapter(Protocol):
    """Structural protocol for adapters that expose capability discovery.

    Any object implementing ``get_capabilities()`` satisfies this protocol
    without needing to inherit from a specific base class.
    """

    def get_capabilities(self) -> List[Any]:
        """Return the list of capabilities supported by this adapter."""
        ...


# ---------------------------------------------------------------------------
# CapabilityMatrix
# ---------------------------------------------------------------------------

class CapabilityMatrix:
    """Cross-reference matrix of adapter capabilities.

    Maps system names to the set of capabilities they support.  Provides
    query helpers and export methods for integration with reporting tools.

    Usage::

        matrix = CapabilityMatrix()
        matrix.register("ThemisDB", my_adapter)
        matrix.register("MockDB", other_adapter)

        # Check support
        assert matrix.supports("ThemisDB", Capability.VECTOR_SEARCH)

        # Which systems support a given capability?
        systems = matrix.systems_supporting(Capability.GRAPH_TRAVERSAL)

        # What does a system support?
        caps = matrix.capabilities_of("ThemisDB")

        # Full matrix as JSON-serialisable dict
        data = matrix.to_dict()
    """

    def __init__(self) -> None:
        # Preserves insertion order; value = list of raw capability values
        self._matrix: Dict[str, List[Any]] = {}

    # ------------------------------------------------------------------
    # Registration
    # ------------------------------------------------------------------

    def register(self, system_name: str, adapter: CapableAdapter) -> None:
        """Register an adapter and record its supported capabilities.

        Args:
            system_name: Vendor-neutral name for the system under test.
            adapter: Any object that implements
                :class:`CapableAdapter` (i.e., exposes
                ``get_capabilities()``).

        Raises:
            TypeError: If *adapter* does not implement ``get_capabilities()``.
            ValueError: If *system_name* is empty.
        """
        if not system_name:
            raise ValueError("system_name must not be empty")
        if not isinstance(adapter, CapableAdapter):
            raise TypeError(
                f"{type(adapter).__name__!r} does not implement get_capabilities()"
            )
        self._matrix[system_name] = list(adapter.get_capabilities())

    def register_capabilities(
        self,
        system_name: str,
        capabilities: Iterable[Any],
    ) -> None:
        """Register a system with an explicit capability list.

        Use this overload when you only have a list of capabilities (e.g.
        loaded from a config file) rather than a live adapter object.

        Args:
            system_name: Vendor-neutral name for the system under test.
            capabilities: Iterable of capability values (strings, enum
                members, or any hashable type).

        Raises:
            ValueError: If *system_name* is empty.
        """
        if not system_name:
            raise ValueError("system_name must not be empty")
        self._matrix[system_name] = list(capabilities)

    # ------------------------------------------------------------------
    # Query helpers
    # ------------------------------------------------------------------

    def supports(self, system_name: str, capability: Any) -> bool:
        """Return *True* if *system_name* supports *capability*.

        Args:
            system_name: Registered system name.
            capability: Capability to check.

        Returns:
            ``True`` if supported, ``False`` if not registered or not
            supported.
        """
        return capability in self._matrix.get(system_name, [])

    def capabilities_of(self, system_name: str) -> List[Any]:
        """Return the capability list for *system_name*.

        Args:
            system_name: Registered system name.

        Returns:
            List of capabilities, empty list if system is unknown.
        """
        return list(self._matrix.get(system_name, []))

    def systems_supporting(self, capability: Any) -> List[str]:
        """Return all system names that support *capability*.

        Returns:
            Alphabetically-sorted list of system names (vendor-neutral
            ordering per CHIMERA specification).
        """
        return sorted(
            name for name, caps in self._matrix.items() if capability in caps
        )

    def registered_systems(self) -> List[str]:
        """Return all registered system names in insertion order."""
        return list(self._matrix.keys())

    def all_capabilities(self) -> List[Any]:
        """Return a deduplicated, sorted list of all known capabilities.

        The sort key is the string representation of each capability so
        that both string and enum values are handled consistently.
        """
        seen = set()
        unique: List[Any] = []
        for caps in self._matrix.values():
            for cap in caps:
                if cap not in seen:
                    seen.add(cap)
                    unique.append(cap)
        return sorted(unique, key=lambda c: str(c))

    # ------------------------------------------------------------------
    # Export
    # ------------------------------------------------------------------

    def to_dict(self) -> Dict[str, Any]:
        """Return a JSON-serialisable representation of the matrix.

        The returned structure is::

            {
                "systems": ["Alpha", "Beta", ...],
                "capabilities": ["cap_a", "cap_b", ...],
                "matrix": {
                    "Alpha": {"cap_a": true, "cap_b": false, ...},
                    "Beta":  {"cap_a": false, "cap_b": true,  ...},
                },
                "coverage": {
                    "cap_a": 1,   # number of systems supporting it
                    "cap_b": 1,
                },
            }

        Capability values are serialised via ``str()`` when they are not
        already JSON-native (e.g. enum members).
        """
        all_caps = self.all_capabilities()
        cap_keys = [_cap_key(c) for c in all_caps]
        cap_map = dict(zip(cap_keys, all_caps))

        systems = self.registered_systems()
        matrix: Dict[str, Dict[str, bool]] = {}
        coverage: Dict[str, int] = {k: 0 for k in cap_keys}

        for system in systems:
            supported = set(self._matrix[system])
            row: Dict[str, bool] = {}
            for key, cap in cap_map.items():
                has = cap in supported
                row[key] = has
                if has:
                    coverage[key] += 1
            matrix[system] = row

        return {
            "systems": systems,
            "capabilities": cap_keys,
            "matrix": matrix,
            "coverage": coverage,
        }

    def to_text_table(
        self,
        supported_mark: str = "✓",
        unsupported_mark: str = "✗",
        column_width: Optional[int] = None,
    ) -> str:
        """Render the matrix as a plain-text table for CLI output.

        Args:
            supported_mark: Character(s) to show when a capability is
                supported.  Defaults to ``"✓"``.
            unsupported_mark: Character(s) to show when a capability is
                not supported.  Defaults to ``"✗"``.
            column_width: Fixed width for each capability column.  When
                ``None`` the width is derived from the longest capability
                key plus two padding characters.

        Returns:
            Multi-line string with header and one row per system.
        """
        data = self.to_dict()
        systems = data["systems"]
        caps = data["capabilities"]

        if not systems or not caps:
            return "(empty matrix)"

        col_w = column_width or (max(len(c) for c in caps) + 2)
        name_w = max(len(s) for s in systems) + 2

        header = "System".ljust(name_w) + "".join(c.ljust(col_w) for c in caps)
        separator = "-" * len(header)
        rows = [header, separator]
        for system in systems:
            row_data = data["matrix"][system]
            row = system.ljust(name_w) + "".join(
                (supported_mark if row_data[c] else unsupported_mark).ljust(col_w)
                for c in caps
            )
            rows.append(row)
        return "\n".join(rows)


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _cap_key(cap: Any) -> str:
    """Return a stable string key for a capability value.

    For enum members the ``.value`` is used when it is a string; otherwise
    ``str()`` is applied so that the result is always JSON-safe.
    """
    if hasattr(cap, "value") and isinstance(cap.value, str):
        return cap.value
    return str(cap)
