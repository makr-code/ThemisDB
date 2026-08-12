"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gen_grpc_web_ts.py                            ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:53:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     877                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fe07567aed  2026-03-21  feat(server): implement gRPC-Web TypeScript client auto-g... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Unit tests for scripts/gen_grpc_web_ts.py

Run with:
    python3 -m pytest tests/test_gen_grpc_web_ts.py -v
    # or from repo root:
    pytest tests/test_gen_grpc_web_ts.py -v
"""

import json
import sys
import textwrap
import tempfile
import importlib.util
from io import StringIO
from pathlib import Path

import pytest

# ---------------------------------------------------------------------------
# Import the generator module directly without requiring it to be installed.
# ---------------------------------------------------------------------------

_SCRIPTS_DIR = Path(__file__).resolve().parent.parent / "scripts"

def _load_module():
    spec = importlib.util.spec_from_file_location(
        "gen_grpc_web_ts",
        _SCRIPTS_DIR / "gen_grpc_web_ts.py",
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

gen = _load_module()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _write_proto(tmp_path: Path, name: str, content: str) -> Path:
    p = tmp_path / name
    p.write_text(textwrap.dedent(content), encoding="utf-8")
    return p


def _parse(content: str, name: str = "test.proto") -> gen.ProtoFile:
    with tempfile.NamedTemporaryFile(
        suffix=".proto", mode="w", encoding="utf-8", delete=False
    ) as f:
        f.write(textwrap.dedent(content))
        tmp = Path(f.name)
    pf = gen.parse_proto_file(tmp)
    tmp.unlink()
    return pf


# ===========================================================================
# Proto parser tests
# ===========================================================================

class TestParseSimpleService:
    """Parser correctly extracts a simple service with unary RPC."""

    PROTO = """\
        syntax = "proto3";
        package acme.v1;

        // Greeting service
        service Greeter {
          // Say hello
          rpc SayHello(HelloRequest) returns (HelloReply);
        }

        message HelloRequest { string name = 1; }
        message HelloReply   { string message = 1; }
    """

    def test_syntax_and_package(self):
        pf = _parse(self.PROTO)
        assert pf.syntax == "proto3"
        assert pf.package == "acme.v1"

    def test_service_extracted(self):
        pf = _parse(self.PROTO)
        assert len(pf.services) == 1
        svc = pf.services[0]
        assert svc.name == "Greeter"

    def test_service_comment(self):
        pf = _parse(self.PROTO)
        assert "Greeting service" in pf.services[0].comment

    def test_rpc_extracted(self):
        pf = _parse(self.PROTO)
        methods = pf.services[0].methods
        assert len(methods) == 1
        m = methods[0]
        assert m.name == "SayHello"
        assert m.request_type == "HelloRequest"
        assert m.response_type == "HelloReply"

    def test_rpc_is_unary(self):
        pf = _parse(self.PROTO)
        m = pf.services[0].methods[0]
        assert not m.client_streaming
        assert not m.server_streaming

    def test_messages_extracted(self):
        pf = _parse(self.PROTO)
        names = [msg.name for msg in pf.messages]
        assert "HelloRequest" in names
        assert "HelloReply" in names

    def test_message_fields(self):
        pf = _parse(self.PROTO)
        req = next(m for m in pf.messages if m.name == "HelloRequest")
        assert len(req.fields) == 1
        f = req.fields[0]
        assert f.name == "name"
        assert f.field_type == "string"
        assert f.number == 1


class TestParseStreamingRPCs:
    """Parser correctly identifies all four streaming flavours."""

    PROTO = """\
        syntax = "proto3";
        package stream.test;
        service StreamSvc {
          rpc Unary(Req) returns (Resp);
          rpc ServerStream(Req) returns (stream Resp);
          rpc ClientStream(stream Req) returns (Resp);
          rpc BidiStream(stream Req) returns (stream Resp);
        }
        message Req  { string data = 1; }
        message Resp { string data = 1; }
    """

    def setup_method(self):
        pf = _parse(self.PROTO)
        self.methods = {m.name: m for m in pf.services[0].methods}

    def test_unary(self):
        m = self.methods["Unary"]
        assert not m.client_streaming and not m.server_streaming

    def test_server_streaming(self):
        m = self.methods["ServerStream"]
        assert not m.client_streaming and m.server_streaming

    def test_client_streaming(self):
        m = self.methods["ClientStream"]
        assert m.client_streaming and not m.server_streaming

    def test_bidi_streaming(self):
        m = self.methods["BidiStream"]
        assert m.client_streaming and m.server_streaming


class TestParseEnums:
    """Parser extracts enum definitions with values."""

    PROTO = """\
        syntax = "proto3";
        package enums;
        // Status codes
        enum Status {
          UNKNOWN = 0;
          OK      = 1;
          FAIL    = 2;
        }
        message Msg { Status status = 1; }
    """

    def test_enum_extracted(self):
        pf = _parse(self.PROTO)
        assert len(pf.enums) == 1
        e = pf.enums[0]
        assert e.name == "Status"

    def test_enum_values(self):
        pf = _parse(self.PROTO)
        vals = {v.name: v.number for v in pf.enums[0].values}
        assert vals == {"UNKNOWN": 0, "OK": 1, "FAIL": 2}

    def test_enum_comment(self):
        pf = _parse(self.PROTO)
        assert "Status codes" in pf.enums[0].comment


class TestParseMapFields:
    """Parser correctly handles map<K, V> fields."""

    PROTO = """\
        syntax = "proto3";
        package maps;
        message Attrs {
          map<string, string> labels = 1;
          map<int32,  float>  counts = 2;
        }
    """

    def test_map_fields_present(self):
        pf = _parse(self.PROTO)
        msg = pf.messages[0]
        field_types = {f.name: f.field_type for f in msg.fields}
        assert "labels" in field_types
        assert "map<" in field_types["labels"]

    def test_map_field_numbers(self):
        pf = _parse(self.PROTO)
        msg = pf.messages[0]
        assert msg.fields[0].number == 1
        assert msg.fields[1].number == 2


class TestParseRepeatedAndOptional:
    """Parser handles repeated and optional qualifiers."""

    PROTO = """\
        syntax = "proto3";
        package quals;
        message Items {
          repeated string tags = 1;
          optional int32 count = 2;
          string name = 3;
        }
    """

    def test_repeated_field(self):
        pf = _parse(self.PROTO)
        f = pf.messages[0].fields[0]
        assert f.repeated is True
        assert f.optional is False

    def test_optional_field(self):
        pf = _parse(self.PROTO)
        f = pf.messages[0].fields[1]
        assert f.optional is True
        assert f.repeated is False

    def test_plain_field(self):
        pf = _parse(self.PROTO)
        f = pf.messages[0].fields[2]
        assert not f.repeated
        assert not f.optional


class TestParseBlockComments:
    """Block comments (/* ... */) are stripped before parsing."""

    PROTO = """\
        /* File header comment */
        syntax = "proto3";
        package blk;
        /* Service comment */
        service Svc {
          /* Method comment */
          rpc Get(Req) returns (Resp);
        }
        message Req  { /* field comment */ string id = 1; }
        message Resp { string val = 1; }
    """

    def test_parsing_succeeds(self):
        pf = _parse(self.PROTO)
        assert pf.package == "blk"
        assert len(pf.services) == 1


class TestParseImports:
    """Parser records import paths."""

    PROTO = """\
        syntax = "proto3";
        import "google/protobuf/timestamp.proto";
        import "common.proto";
        package imp;
        service S { rpc M(R) returns (R); }
        message R {}
    """

    def test_imports_collected(self):
        pf = _parse(self.PROTO)
        assert "google/protobuf/timestamp.proto" in pf.imports
        assert "common.proto" in pf.imports


class TestParseMultipleServices:
    """Multiple services in one file are all extracted."""

    PROTO = """\
        syntax = "proto3";
        package multi;
        service Alpha { rpc A(M) returns (M); }
        service Beta  { rpc B(M) returns (M); }
        message M {}
    """

    def test_two_services(self):
        pf = _parse(self.PROTO)
        names = [s.name for s in pf.services]
        assert "Alpha" in names
        assert "Beta" in names


# ===========================================================================
# Type-mapping tests
# ===========================================================================

class TestProtoTypeToTs:
    """_proto_type_to_ts converts scalar and complex types correctly."""

    def _conv(self, t: str) -> str:
        return gen._proto_type_to_ts(t, set(), set())

    def test_string(self):      assert self._conv("string")  == "string"
    def test_bool(self):        assert self._conv("bool")    == "boolean"
    def test_int32(self):       assert self._conv("int32")   == "number"
    def test_int64_string(self):assert self._conv("int64")   == "string"
    def test_bytes(self):       assert self._conv("bytes")   == "Uint8Array"
    def test_double(self):      assert self._conv("double")  == "number"
    def test_float(self):       assert self._conv("float")   == "number"
    def test_uint32(self):      assert self._conv("uint32")  == "number"

    def test_map_string_string(self):
        result = gen._proto_type_to_ts("map<string,string>", set(), set())
        assert "{ [key: string]: string }" == result

    def test_map_int32_string(self):
        result = gen._proto_type_to_ts("map<int32,string>", set(), set())
        assert "{ [key: number]: string }" == result

    def test_message_ref(self):
        result = gen._proto_type_to_ts("MyMessage", {"MyMessage"}, set())
        assert result == "MyMessage"

    def test_qualified_message_ref(self):
        result = gen._proto_type_to_ts("pkg.sub.MyMessage", {"MyMessage"}, set())
        assert result == "MyMessage"


# ===========================================================================
# Code generation tests
# ===========================================================================

class TestGenerateMessageTs:
    """generate_messages_ts produces valid TypeScript interface declarations."""

    PROTO = """\
        syntax = "proto3";
        package demo;
        enum Color { RED = 0; GREEN = 1; BLUE = 2; }
        message Shape {
          string name   = 1;
          int32  sides  = 2;
          Color  color  = 3;
          repeated float vertices = 4;
        }
    """

    def setup_method(self):
        pf = _parse(self.PROTO)
        self.ts = gen.generate_messages_ts([pf])

    def test_enum_emitted(self):
        assert "export enum Color" in self.ts

    def test_enum_values_emitted(self):
        assert "RED = 0" in self.ts
        assert "GREEN = 1" in self.ts
        assert "BLUE = 2" in self.ts

    def test_interface_emitted(self):
        assert "export interface Shape" in self.ts

    def test_string_field(self):
        assert "name?" in self.ts

    def test_int32_field(self):
        assert "sides?" in self.ts

    def test_repeated_field_array(self):
        assert "vertices?" in self.ts
        assert "number[]" in self.ts

    def test_no_edit_warning(self):
        assert "DO NOT EDIT" in self.ts


class TestGenerateServiceTs:
    """generate_service_ts produces correct TypeScript client classes."""

    PROTO = """\
        syntax = "proto3";
        package acme;
        service Calculator {
          // Add two numbers
          rpc Add(AddReq) returns (AddResp);
          rpc StreamNumbers(NumReq) returns (stream NumResp);
        }
        message AddReq  { int32 a = 1; int32 b = 2; }
        message AddResp { int32 result = 1; }
        message NumReq  { int32 count = 1; }
        message NumResp { int32 value = 1; }
    """

    def setup_method(self):
        pf = _parse(self.PROTO)
        svc = pf.services[0]
        self.ts = gen.generate_service_ts(
            svc, pf.package, {"AddReq", "AddResp", "NumReq", "NumResp"}, set()
        )

    def test_class_declaration(self):
        assert "export class CalculatorClient" in self.ts

    def test_grpc_web_import(self):
        assert 'import * as grpc from "grpc-web"' in self.ts

    def test_unary_method_promise(self):
        assert "add(" in self.ts
        assert "Promise<msg.AddResp>" in self.ts

    def test_server_streaming_observable(self):
        assert "streamNumbers(" in self.ts
        assert "Observable<msg.NumResp>" in self.ts

    def test_constructor_present(self):
        assert "constructor(" in self.ts

    def test_method_descriptor_unary(self):
        assert "grpc.MethodType.UNARY" in self.ts

    def test_method_descriptor_server_stream(self):
        assert "grpc.MethodType.SERVER_STREAMING" in self.ts

    def test_service_base_url_constant(self):
        assert "_SERVICE = 'acme.Calculator'" in self.ts

    def test_method_comment_propagated(self):
        assert "Add two numbers" in self.ts


class TestGenerateIndexTs:
    """generate_index_ts re-exports all service clients."""

    PROTO = """\
        syntax = "proto3";
        package idx;
        service Alpha { rpc A(M) returns (M); }
        service Beta  { rpc B(M) returns (M); }
        message M {}
    """

    def setup_method(self):
        pf = _parse(self.PROTO)
        self.ts = gen.generate_index_ts([pf])

    def test_messages_exported(self):
        assert "export * from './messages'" in self.ts

    def test_alpha_client_exported(self):
        assert "AlphaClient" in self.ts

    def test_beta_client_exported(self):
        assert "BetaClient" in self.ts


class TestGeneratePackageJson:
    """generate_package_json produces a valid npm manifest."""

    def test_name_in_manifest(self):
        pkg = json.loads(gen.generate_package_json("@foo/bar", "2.3.4"))
        assert pkg["name"] == "@foo/bar"

    def test_version_in_manifest(self):
        pkg = json.loads(gen.generate_package_json("@foo/bar", "2.3.4"))
        assert pkg["version"] == "2.3.4"

    def test_grpc_web_dependency(self):
        pkg = json.loads(gen.generate_package_json("@foo/bar"))
        assert "grpc-web" in pkg["dependencies"]

    def test_rxjs_dependency(self):
        pkg = json.loads(gen.generate_package_json("@foo/bar"))
        assert "rxjs" in pkg["dependencies"]

    def test_types_field(self):
        pkg = json.loads(gen.generate_package_json("@foo/bar"))
        assert pkg["types"].endswith(".d.ts")


class TestGenerateTsConfigJson:
    """generate_tsconfig_json produces a valid TypeScript configuration."""

    def test_valid_json(self):
        tsconf = json.loads(gen.generate_tsconfig_json())
        assert "compilerOptions" in tsconf

    def test_declaration_enabled(self):
        tsconf = json.loads(gen.generate_tsconfig_json())
        assert tsconf["compilerOptions"]["declaration"] is True

    def test_strict_enabled(self):
        tsconf = json.loads(gen.generate_tsconfig_json())
        assert tsconf["compilerOptions"]["strict"] is True


# ===========================================================================
# File-writing / full-pipeline tests
# ===========================================================================

class TestGenerateToDirectory:
    """Full pipeline: parse proto files → write TypeScript package to disk."""

    PROTO = """\
        syntax = "proto3";
        package myapp.v1;
        service Greet {
          rpc Hello(HelloReq) returns (HelloResp);
        }
        message HelloReq  { string name = 1; }
        message HelloResp { string greeting = 1; }
    """

    def test_files_created(self, tmp_path):
        proto_file = _write_proto(tmp_path, "greet.proto", self.PROTO)
        pf = gen.parse_proto_file(proto_file)
        out_dir = tmp_path / "out"
        gen.generate([pf], out_dir=out_dir, dry_run=False)

        assert (out_dir / "package.json").exists()
        assert (out_dir / "tsconfig.json").exists()
        assert (out_dir / "src" / "messages.ts").exists()
        assert (out_dir / "src" / "index.ts").exists()
        assert (out_dir / "src" / "Greet.ts").exists()

    def test_messages_ts_has_interface(self, tmp_path):
        proto_file = _write_proto(tmp_path, "greet.proto", self.PROTO)
        pf = gen.parse_proto_file(proto_file)
        out_dir = tmp_path / "out"
        gen.generate([pf], out_dir=out_dir, dry_run=False)
        content = (out_dir / "src" / "messages.ts").read_text()
        assert "export interface HelloReq" in content

    def test_service_ts_has_client_class(self, tmp_path):
        proto_file = _write_proto(tmp_path, "greet.proto", self.PROTO)
        pf = gen.parse_proto_file(proto_file)
        out_dir = tmp_path / "out"
        gen.generate([pf], out_dir=out_dir, dry_run=False)
        content = (out_dir / "src" / "Greet.ts").read_text()
        assert "export class GreetClient" in content

    def test_index_ts_re_exports_client(self, tmp_path):
        proto_file = _write_proto(tmp_path, "greet.proto", self.PROTO)
        pf = gen.parse_proto_file(proto_file)
        out_dir = tmp_path / "out"
        gen.generate([pf], out_dir=out_dir, dry_run=False)
        content = (out_dir / "src" / "index.ts").read_text()
        assert "GreetClient" in content

    def test_package_json_name(self, tmp_path):
        proto_file = _write_proto(tmp_path, "greet.proto", self.PROTO)
        pf = gen.parse_proto_file(proto_file)
        out_dir = tmp_path / "out"
        gen.generate([pf], out_dir=out_dir, package_name="@test/pkg", dry_run=False)
        pkg = json.loads((out_dir / "package.json").read_text())
        assert pkg["name"] == "@test/pkg"

    def test_dry_run_prints_to_stdout(self, tmp_path, capsys):
        proto_file = _write_proto(tmp_path, "greet.proto", self.PROTO)
        pf = gen.parse_proto_file(proto_file)
        out_dir = tmp_path / "out"
        gen.generate([pf], out_dir=out_dir, dry_run=True)
        captured = capsys.readouterr()
        assert "GreetClient" in captured.out
        assert not out_dir.exists()  # nothing written


# ===========================================================================
# CLI tests
# ===========================================================================

class TestCLI:
    """main() CLI function returns correct exit codes."""

    PROTO = """\
        syntax = "proto3";
        package cli.test;
        service Pong { rpc Ping(PingReq) returns (PingResp); }
        message PingReq  { string id = 1; }
        message PingResp { bool ok = 1; }
    """

    def test_exit_zero_on_success(self, tmp_path):
        proto_dir = tmp_path / "proto"
        proto_dir.mkdir()
        _write_proto(proto_dir, "ping.proto", self.PROTO)
        out_dir = tmp_path / "out"
        rc = gen.main([
            "--proto-dir", str(proto_dir),
            "--out-dir", str(out_dir),
        ])
        assert rc == 0

    def test_exit_zero_dry_run(self, tmp_path):
        proto_dir = tmp_path / "proto"
        proto_dir.mkdir()
        _write_proto(proto_dir, "ping.proto", self.PROTO)
        rc = gen.main([
            "--proto-dir", str(proto_dir),
            "--out-dir", str(tmp_path / "out"),
            "--dry-run",
        ])
        assert rc == 0

    def test_exit_one_missing_proto_dir(self, tmp_path):
        rc = gen.main(["--proto-dir", str(tmp_path / "nonexistent")])
        assert rc == 1

    def test_exit_zero_empty_proto_dir(self, tmp_path):
        proto_dir = tmp_path / "proto"
        proto_dir.mkdir()
        rc = gen.main(["--proto-dir", str(proto_dir)])
        assert rc == 0

    def test_custom_package_name_in_output(self, tmp_path):
        proto_dir = tmp_path / "proto"
        proto_dir.mkdir()
        _write_proto(proto_dir, "ping.proto", self.PROTO)
        out_dir = tmp_path / "out"
        gen.main([
            "--proto-dir", str(proto_dir),
            "--out-dir", str(out_dir),
            "--package-name", "@custom/pkg",
        ])
        pkg = json.loads((out_dir / "package.json").read_text())
        assert pkg["name"] == "@custom/pkg"

    def test_version_flag(self, tmp_path):
        proto_dir = tmp_path / "proto"
        proto_dir.mkdir()
        _write_proto(proto_dir, "ping.proto", self.PROTO)
        out_dir = tmp_path / "out"
        gen.main([
            "--proto-dir", str(proto_dir),
            "--out-dir", str(out_dir),
            "--version", "3.1.4",
        ])
        pkg = json.loads((out_dir / "package.json").read_text())
        assert pkg["version"] == "3.1.4"


# ===========================================================================
# Integration: parse the real proto files from the repository
# ===========================================================================

class TestRealProtoFiles:
    """Integration test: parse all proto files in the repo's proto/ directory."""

    _REPO_ROOT = Path(__file__).resolve().parent.parent
    _PROTO_DIR = _REPO_ROOT / "proto"

    def _get_proto_files(self):
        if not self._PROTO_DIR.is_dir():
            pytest.skip(f"proto/ directory not found at {self._PROTO_DIR}")
        return sorted(self._PROTO_DIR.glob("**/*.proto"))

    def test_all_proto_files_parse_without_error(self):
        for p in self._get_proto_files():
            pf = gen.parse_proto_file(p)
            # Just check it parsed without exception and syntax is recognised
            assert pf.syntax in ("proto3", "proto2"), f"{p}: unexpected syntax {pf.syntax!r}"

    def test_all_proto3_files_have_package(self):
        for p in self._get_proto_files():
            pf = gen.parse_proto_file(p)
            if pf.syntax == "proto3":
                assert pf.package, f"{p}: proto3 file has no package declaration"

    def test_generation_completes_fast(self):
        """Full generation for all repo proto files must finish in ≤ 5 s."""
        import time
        proto_files = []
        for p in self._get_proto_files():
            pf = gen.parse_proto_file(p)
            if pf.syntax == "proto3":
                proto_files.append(pf)

        start = time.monotonic()
        with tempfile.TemporaryDirectory() as out:
            gen.generate(proto_files, Path(out), dry_run=False)
        elapsed = time.monotonic() - start
        assert elapsed < 5.0, f"Generation took {elapsed:.2f}s (limit: 5s)"

    def test_generated_index_exports_all_services(self):
        proto_files = []
        for p in self._get_proto_files():
            pf = gen.parse_proto_file(p)
            if pf.syntax == "proto3":
                proto_files.append(pf)

        index_ts = gen.generate_index_ts(proto_files)
        for pf in proto_files:
            for svc in pf.services:
                assert f"{svc.name}Client" in index_ts, (
                    f"Expected {svc.name}Client export in index.ts"
                )

    def test_generated_messages_exports_all_interfaces(self):
        proto_files = []
        for p in self._get_proto_files():
            pf = gen.parse_proto_file(p)
            if pf.syntax == "proto3":
                proto_files.append(pf)

        messages_ts = gen.generate_messages_ts(proto_files)
        # Spot-check: every top-level message should appear as an interface
        for pf in proto_files:
            for msg in pf.messages:
                assert f"export interface {msg.name}" in messages_ts, (
                    f"Expected interface {msg.name} in messages.ts"
                )


# ===========================================================================
# Edge-case tests
# ===========================================================================

class TestEdgeCases:
    """Edge cases and robustness tests for the parser and generator."""

    def test_empty_service(self):
        """An empty service body (no methods) parses without error."""
        pf = _parse("""\
            syntax = "proto3";
            package empty;
            service EmptySvc {}
            message M {}
        """)
        assert len(pf.services) == 1
        assert pf.services[0].methods == []

    def test_empty_message(self):
        """An empty message parses without error."""
        pf = _parse("""\
            syntax = "proto3";
            package empty;
            service S { rpc M(Empty) returns (Empty); }
            message Empty {}
        """)
        assert len(pf.messages) == 1
        assert pf.messages[0].fields == []

    def test_comment_before_service(self):
        """Leading comment before service is captured."""
        pf = _parse("""\
            syntax = "proto3";
            package c;
            // My Service
            service MySvc { rpc M(R) returns (R); }
            message R {}
        """)
        assert "My Service" in pf.services[0].comment

    def test_comment_before_message(self):
        """Leading comment before message is captured."""
        pf = _parse("""\
            syntax = "proto3";
            package c;
            service S { rpc M(MyMsg) returns (MyMsg); }
            // The request message
            message MyMsg { string id = 1; }
        """)
        msg = next(m for m in pf.messages if m.name == "MyMsg")
        assert "The request message" in msg.comment

    def test_option_lines_skipped(self):
        """option statements do not interfere with parsing."""
        pf = _parse("""\
            syntax = "proto3";
            package opts;
            option cc_enable_arenas = true;
            option optimize_for = SPEED;
            service S { rpc M(R) returns (R); }
            message R { option deprecated = true; string x = 1; }
        """)
        assert pf.package == "opts"
        assert len(pf.services) == 1
        # "x" field should be present even with option in message body
        msg = pf.messages[0]
        names = [f.name for f in msg.fields]
        assert "x" in names

    def test_oneof_fields_included(self):
        """Fields inside a oneof block are still collected."""
        pf = _parse("""\
            syntax = "proto3";
            package oo;
            service S { rpc M(R) returns (R); }
            message R {
              oneof payload {
                string text  = 1;
                bytes  blob  = 2;
              }
            }
        """)
        msg = pf.messages[0]
        field_names = [f.name for f in msg.fields]
        assert "text" in field_names
        assert "blob" in field_names

    def test_qualified_return_type(self):
        """Qualified type names (pkg.Sub.Type) are resolved to local part."""
        pf = _parse("""\
            syntax = "proto3";
            package qual;
            service S { rpc M(pkg.sub.Req) returns (pkg.sub.Resp); }
            message Req  {}
            message Resp {}
        """)
        m = pf.services[0].methods[0]
        assert m.request_type == "pkg.sub.Req"
        assert m.response_type == "pkg.sub.Resp"
        # TypeScript generation should strip the prefix
        ts_req = gen._proto_type_to_ts(m.request_type, set(), set())
        assert ts_req == "Req"

    def test_jsdoc_single_line(self):
        comment = "A simple description."
        result = gen._jsdoc(comment)
        assert "/** A simple description. */" in result

    def test_jsdoc_empty(self):
        assert gen._jsdoc("") == ""

    def test_ts_identifier_lowercases_first_char(self):
        assert gen._ts_identifier("SayHello") == "sayHello"
        assert gen._ts_identifier("Get") == "get"
        assert gen._ts_identifier("") == ""

    def test_flatten_messages_nested(self):
        """Nested messages get underscored flat names."""
        outer = gen.ProtoMessage(name="Outer")
        inner = gen.ProtoMessage(name="Inner")
        outer.nested = [inner]
        flat = gen._flatten_messages([outer])
        names = [m.flat_name for m in flat]
        assert "Outer" in names
        assert "Outer_Inner" in names
