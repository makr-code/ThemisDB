import contextlib
import ctypes
import json
from pathlib import Path
import shutil
import subprocess
import types
import unittest
from unittest.mock import Mock, patch
import uuid

import httpx

from benchmarks import core_http_restart as runner
from benchmarks.windows_private_console import _Console, _send_checked_ctrl_c


class ConsoleSafetyTests(unittest.TestCase):
    def console(self, snapshots=None):
        console = Mock()
        console.members.side_effect = snapshots or [[11, 22]] * 3
        return console

    def test_only_exact_owned_pair_permits_console_broadcast(self):
        console, identity = self.console(), Mock()
        evidence = _send_checked_ctrl_c(console, 11, 22, identity)
        self.assertTrue(evidence["event_sent"])
        self.assertEqual(len(evidence["members_before"]), 3)
        console.generate_ctrl_c.assert_called_once_with()
        console.detach.assert_called_once_with()
        self.assertEqual(identity.call_count, 2)

    def test_unexpected_or_changed_membership_never_sends(self):
        for rejected in ([11, 22, 33], [11], [22], [], [11, 11], [11, 33]):
            for position in range(3):
                with self.subTest(rejected=rejected, position=position):
                    console = self.console([[11, 22]] * position + [rejected])
                    with self.assertRaises(RuntimeError):
                        _send_checked_ctrl_c(console, 11, 22, Mock())
                    console.generate_ctrl_c.assert_not_called()
                    console.detach.assert_called_once_with()

    def test_detached_identity_and_attachment_failures_never_send(self):
        for operation in ("require_detached", "attach", "ignore_ctrl_c", "members"):
            with self.subTest(operation=operation):
                console = self.console()
                getattr(console, operation).side_effect = OSError("denied")
                with self.assertRaises(OSError):
                    _send_checked_ctrl_c(console, 11, 22, Mock())
                console.generate_ctrl_c.assert_not_called()
        for position in (0, 1):
            console, identity = self.console(), Mock()
            identity.side_effect = [None] * position + [RuntimeError("PID reused")]
            with self.assertRaises(RuntimeError):
                _send_checked_ctrl_c(console, 11, 22, identity)
            console.generate_ctrl_c.assert_not_called()

    def test_overflow_and_empty_api_results_fail_closed(self):
        console = object.__new__(_Console)
        console.dword = ctypes.c_uint32
        console.api = Mock()
        console.api.GetConsoleProcessList.return_value = 65
        with self.assertRaisesRegex(RuntimeError, "overflow"):
            console.members()
        console.api.GetConsoleProcessList.return_value = 0
        with patch.object(console, "_checked", side_effect=OSError("API failure")):
            with self.assertRaises(OSError):
                console.members()

    def test_ctrl_c_is_console_scoped_not_process_group_scoped(self):
        console = object.__new__(_Console)
        console.api = Mock()
        console.generate_ctrl_c()
        console.api.GenerateConsoleCtrlEvent.assert_called_once_with(0, 0)


class ProofTests(unittest.TestCase):
    def test_handler_completion_and_normal_exit_are_all_required(self):
        for code, stdout, stderr in (
            (3221225786, runner.COMPLETE_MESSAGE, runner.HANDLER_MESSAGE),
            (1, runner.COMPLETE_MESSAGE, runner.HANDLER_MESSAGE),
            (0, runner.COMPLETE_MESSAGE, ""),
            (0, "", runner.HANDLER_MESSAGE),
        ):
            with self.subTest(code=code, stdout=stdout, stderr=stderr):
                with self.assertRaises(RuntimeError):
                    runner._exit_proof(code, stdout, stderr)
        self.assertEqual(runner._exit_proof(
            0, runner.COMPLETE_MESSAGE, runner.HANDLER_MESSAGE)["exit_code"], 0)

    def test_readback_requires_exact_key_blob_and_types(self):
        expected = {"n": 1, "text": "\u00fc"}
        invalid = [
            {"key": "other", "blob": json.dumps(expected)},
            {"key": "owned", "blob": json.dumps({"n": True, "text": "\u00fc"})},
            {"key": "owned", "blob": "invalid"},
            {"key": "owned"}, [], {"key": "owned", "blob": "{}"},
        ]
        for payload in invalid:
            with self.subTest(payload=payload):
                client = Mock()
                client.get.return_value = httpx.Response(
                    200, json=payload, request=httpx.Request("GET", "http://localhost"))
                with self.assertRaises(RuntimeError):
                    runner._read(client, "owned", expected)

    def test_nonloopback_listener_is_rejected(self):
        process = Mock()
        process.net_connections.return_value = [
            types.SimpleNamespace(status="LISTEN", laddr=types.SimpleNamespace(
                ip="0.0.0.0", port=12345))]
        with self.assertRaisesRegex(RuntimeError, "non-loopback"):
            runner._verify_listeners(process, [12345])

    def test_stop_timeout_forbids_restart(self):
        owned = runner._OwnedServer(Path.cwd(), Path("unused.exe"), Path("unused.json"),
                                    {}, [12345, 12346], "test", 1, 1)
        owned.process = Mock(pid=11)
        owned.process.poll.return_value = None
        owned.process.wait.side_effect = subprocess.TimeoutExpired("owned", 1)
        helper = Mock(pid=22, returncode=0)
        helper.communicate.return_value = (json.dumps({
            "event_sent": True, "server_pid": 11, "helper_pid": 22,
            "members_before": [[11, 22]] * 3}).encode(), b"")
        with patch.object(runner.subprocess, "Popen", return_value=helper), \
                patch.object(runner.subprocess, "DETACHED_PROCESS", 8, create=True), \
                patch.object(runner.psutil, "Process") as process:
            process.return_value.create_time.return_value = 1
            with self.assertRaisesRegex(RuntimeError, "restart forbidden"):
                owned.stop()


@contextlib.contextmanager
def workspace():
    directory = Path.cwd() / ("restart-test-" + uuid.uuid4().hex)
    directory.mkdir()
    try:
        yield directory
    finally:
        shutil.rmtree(directory)


class LifecycleTests(unittest.TestCase):
    def exercise(self, directory, stop_error=None):
        source = directory / "source.exe"
        source.write_bytes(b"test executable never launched")
        server = Mock()
        server.start.side_effect = [{"pid": 11}, {"pid": 33}]
        server.stop.side_effect = stop_error or [{"exit_code": 0}, {"exit_code": 0}]
        entities, requests = {}, []

        def transport(request):
            key = request.url.path.removeprefix("/entities/")
            requests.append((request.method, key))
            if request.method == "PUT":
                entities[key] = json.loads(request.content)["blob"]
            if request.method == "DELETE":
                entities.pop(key)
            if request.method == "GET":
                if key not in entities:
                    return httpx.Response(404)
                return httpx.Response(200, json={"key": key, "blob": entities[key]})
            return httpx.Response(200)

        client = httpx.Client(transport=httpx.MockTransport(transport),
                             base_url="http://127.0.0.1")
        with contextlib.closing(client), patch.object(runner.sys, "platform", "win32"), \
                patch.object(runner.sys, "getwindowsversion",
                             return_value=types.SimpleNamespace(platform_version=(10, 0, 0)),
                             create=True), \
                patch.object(runner, "_OwnedServer", return_value=server) as factory, \
                patch.object(runner.httpx, "Client", return_value=client), \
                patch.object(runner, "_port", side_effect=[12345, 12346, 12347]):
            if stop_error:
                with self.assertRaisesRegex(RuntimeError, "missing proof"):
                    runner.run_check(source, directory, allow_private_windows_restart=True)
                result = None
            else:
                result = runner.run_check(source, directory, allow_private_windows_restart=True)
            runtime, executable = factory.call_args.args[:2]
            self.assertEqual(runtime.parent.parent, directory)
            self.assertEqual(runtime.name, "runtime")
            self.assertEqual(executable.parent, runtime)
            self.assertNotEqual(runtime.parent, directory)  # ../logs stays private.
        return result, server, entities, requests

    def test_no_restart_after_failed_stop_and_private_files_removed(self):
        with workspace() as directory:
            _, server, _, _ = self.exercise(directory, RuntimeError("missing proof"))
            self.assertEqual(server.start.call_count, 1)
            server.dispose.assert_called_once_with()
            self.assertEqual(list(directory.iterdir()), [directory / "source.exe"])

    def test_only_owned_keys_are_deleted_after_restart(self):
        with workspace() as directory:
            report, server, entities, requests = self.exercise(directory)
            self.assertTrue(report["restart_readback"])
            self.assertTrue(report["private_directory_removed"])
            self.assertEqual(server.start.call_count, 2)
            self.assertEqual(server.stop.call_count, 2)
            self.assertEqual(entities, {})
            deleted = [key for method, key in requests if method == "DELETE"]
            self.assertEqual(deleted, report["owned_keys_deleted_and_absent"])
            self.assertTrue(all(key.startswith("restart_" + report["run_id"] + ":")
                                for _, key in requests))

    def test_opt_in_and_platform_fail_before_files_or_processes(self):
        with patch.object(runner.subprocess, "Popen") as process:
            with self.assertRaisesRegex(ValueError, "opt-in"):
                runner.run_check("missing.exe", "missing")
            with patch.object(runner.sys, "platform", "linux"):
                with self.assertRaisesRegex(RuntimeError, "Windows-only"):
                    runner.run_check("missing.exe", "missing",
                                     allow_private_windows_restart=True)
            process.assert_not_called()

    def test_changing_build_input_is_rejected_before_launch(self):
        with workspace() as directory:
            source = directory / "source.exe"
            source.write_bytes(b"before")
            copy = runner.shutil.copy2

            def changing_copy(first, last):
                copy(first, last)
                source.write_bytes(b"changed by concurrent build")

            with patch.object(runner.sys, "platform", "win32"), \
                    patch.object(runner.shutil, "copy2", side_effect=changing_copy), \
                    patch.object(runner, "_OwnedServer") as server:
                with self.assertRaisesRegex(RuntimeError, "changed during copy"):
                    runner.run_check(source, directory, allow_private_windows_restart=True)
                server.assert_not_called()
            self.assertEqual(list(directory.iterdir()), [source])

    def test_cli_never_overwrites_existing_report(self):
        with workspace() as directory:
            output = directory / "report.json"
            output.write_text("existing report", encoding="utf-8")
            with patch.object(runner, "run_check") as run:
                self.assertEqual(runner.main([
                    "--server-exe", "unused.exe", "--output", str(output)]), 1)
                run.assert_not_called()
            self.assertEqual(output.read_text(), "existing report")

    def test_failed_cli_removes_reserved_report(self):
        with workspace() as directory:
            output = directory / "report.json"
            with patch.object(runner, "run_check", side_effect=RuntimeError("no proof")):
                self.assertEqual(runner.main([
                    "--server-exe", "unused.exe", "--output", str(output)]), 1)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
