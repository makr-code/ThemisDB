#!/usr/bin/env python3
"""Internal detached helper for the owned Core HTTP restart check.

Never attach the calling shell or use this with a caller-selected server PID.
Console membership is a snapshot, not protection against hostile AttachConsole
calls by another process running as the same user.
"""

from __future__ import annotations

import ctypes
import json
import os
from pathlib import Path
import sys

import psutil


class _Console:
    def __init__(self):
        if os.name != "nt":
            raise RuntimeError("Private console shutdown is Windows-only")
        from ctypes import wintypes

        self.dword = wintypes.DWORD
        self.api = ctypes.WinDLL("kernel32", use_last_error=True)
        for name, arguments, result in (
            ("AttachConsole", [wintypes.DWORD], wintypes.BOOL),
            ("FreeConsole", [], wintypes.BOOL),
            ("GetConsoleProcessList",
             [ctypes.POINTER(wintypes.DWORD), wintypes.DWORD], wintypes.DWORD),
            ("SetConsoleCtrlHandler", [ctypes.c_void_p, wintypes.BOOL], wintypes.BOOL),
            ("GenerateConsoleCtrlEvent", [wintypes.DWORD, wintypes.DWORD], wintypes.BOOL),
        ):
            function = getattr(self.api, name)
            function.argtypes, function.restype = arguments, result

    @staticmethod
    def _checked(result):
        if not result:
            raise ctypes.WinError(ctypes.get_last_error())
        return result

    def require_detached(self):
        buffer = (self.dword * 64)()
        ctypes.set_last_error(0)
        count = self.api.GetConsoleProcessList(buffer, len(buffer))
        if count or ctypes.get_last_error() != 6:  # ERROR_INVALID_HANDLE
            raise RuntimeError("Helper is not verifiably detached; refusing to signal")

    def attach(self, pid):
        self._checked(self.api.AttachConsole(pid))

    def ignore_ctrl_c(self):
        # AttachConsole resets handlers; ignore only in this short-lived helper.
        self._checked(self.api.SetConsoleCtrlHandler(None, True))

    def members(self):
        buffer = (self.dword * 64)()
        count = self._checked(self.api.GetConsoleProcessList(buffer, len(buffer)))
        if count > len(buffer):
            raise RuntimeError("Console process list overflow; refusing to signal")
        return list(buffer[:count])

    def generate_ctrl_c(self):
        # CTRL_C_EVENT cannot target a process group. Zero broadcasts ONLY to
        # the caller's attached console, whose membership was just checked.
        self._checked(self.api.GenerateConsoleCtrlEvent(0, 0))

    def detach(self):
        self._checked(self.api.FreeConsole())


def _send_checked_ctrl_c(console, server_pid, helper_pid, check_identity):
    if (server_pid <= 0 or helper_pid <= 0 or server_pid == helper_pid):
        raise RuntimeError("Invalid owned console identities")
    console.require_detached()
    check_identity()
    console.attach(server_pid)
    evidence = {"server_pid": server_pid, "helper_pid": helper_pid,
                "event_sent": False, "members_before": []}
    try:
        console.ignore_ctrl_c()
        for _ in range(2):
            members = console.members()
            evidence["members_before"].append(members)
            if len(members) != 2 or set(members) != {server_pid, helper_pid}:
                raise RuntimeError("Unexpected console member; no Ctrl+C sent")
        check_identity()
        members = console.members()
        evidence["members_before"].append(members)
        if len(members) != 2 or set(members) != {server_pid, helper_pid}:
            raise RuntimeError("Console membership changed; no Ctrl+C sent")
        console.generate_ctrl_c()
        evidence["event_sent"] = True
        return evidence
    finally:
        console.detach()


def _main(arguments):
    if os.name != "nt" or len(arguments) != 4:
        raise RuntimeError("Internal Windows helper requires owned process identities")
    target, parent = int(arguments[0]), int(arguments[1])
    created, executable = float(arguments[2]), Path(arguments[3]).resolve(strict=True)
    me, server = psutil.Process(), psutil.Process(target)

    def check_identity():
        if (target == parent or me.ppid() != parent or server.ppid() != parent
                or server.create_time() != created or not server.is_running()
                or Path(server.exe()).resolve(strict=True) != executable):
            raise RuntimeError("Owned-process identity mismatch; no Ctrl+C sent")

    return _send_checked_ctrl_c(_Console(), target, os.getpid(), check_identity)


if __name__ == "__main__":
    try:
        print(json.dumps(_main(sys.argv[1:])), flush=True)
    except Exception as exc:
        # A failure after GenerateConsoleCtrlEvent (e.g. FreeConsole failing)
        # must not falsely assert that no event was generated.
        print(json.dumps({"event_sent": None, "error": str(exc)}), flush=True)
        sys.exit(1)
