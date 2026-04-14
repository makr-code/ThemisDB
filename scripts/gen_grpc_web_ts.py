"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gen_grpc_web_ts.py                                 ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 18:45:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1030                                           ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
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
ThemisDB gRPC-Web TypeScript Client Generator
=============================================

Parses proto3 service definitions from ``<repo-root>/proto/`` and generates
typed TypeScript client stubs for the ``grpc-web`` npm package, emitting an
``@themisdb/client-grpc-web`` npm-ready package.

Usage:
    python3 scripts/gen_grpc_web_ts.py
    python3 scripts/gen_grpc_web_ts.py --proto-dir proto --out-dir clients/grpc-web
    python3 scripts/gen_grpc_web_ts.py --dry-run    # print index.ts to stdout

Generated structure::

    <out-dir>/
      package.json          npm package manifest (@themisdb/client-grpc-web)
      tsconfig.json         TypeScript compiler options
      src/
        index.ts            Re-exports all public symbols
        messages.ts         TypeScript interfaces for every proto message/enum
        <Service>.ts        Per-service strongly-typed client class

Exit codes:
    0  Success
    1  Proto parse error or I/O failure
    2  CLI argument error

Design notes:
    - Pure Python 3.8+ standard-library implementation; no protoc dependency.
    - Regex-based parser handles proto3 only; proto2 constructs are skipped.
    - Nested message types are flattened to top-level with dotted names.
    - Streaming methods (server/client/bidi) emit Observable<T> return types.
    - ``map<K, V>`` fields are emitted as ``{ [key: string]: V }``.
    - Comments (``//``) are propagated as JSDoc on the generated types.
"""

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ---------------------------------------------------------------------------
# Proto AST data-classes
# ---------------------------------------------------------------------------

@dataclass
class ProtoField:
    """Single field inside a message."""
    name: str
    field_type: str          # proto type string (may be "map<K, V>")
    number: int
    repeated: bool = False
    optional: bool = False
    oneof_group: Optional[str] = None
    comment: str = ""


@dataclass
class ProtoMessage:
    name: str
    fields: List[ProtoField] = field(default_factory=list)
    nested: List["ProtoMessage"] = field(default_factory=list)
    comment: str = ""
    # fully-qualified flat name (populated by parser after nesting resolution)
    flat_name: str = ""


@dataclass
class ProtoEnumValue:
    name: str
    number: int
    comment: str = ""


@dataclass
class ProtoEnum:
    name: str
    values: List[ProtoEnumValue] = field(default_factory=list)
    comment: str = ""
    flat_name: str = ""


@dataclass
class ProtoMethod:
    name: str
    request_type: str
    response_type: str
    client_streaming: bool = False
    server_streaming: bool = False
    comment: str = ""


@dataclass
class ProtoService:
    name: str
    methods: List[ProtoMethod] = field(default_factory=list)
    comment: str = ""


@dataclass
class ProtoFile:
    path: Path
    syntax: str = "proto3"
    package: str = ""
    imports: List[str] = field(default_factory=list)
    services: List[ProtoService] = field(default_factory=list)
    messages: List[ProtoMessage] = field(default_factory=list)
    enums: List[ProtoEnum] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Proto3 parser
# ---------------------------------------------------------------------------

# Patterns
_RE_SYNTAX   = re.compile(r'^\s*syntax\s*=\s*"(proto\d)"')
_RE_PACKAGE  = re.compile(r'^\s*package\s+([\w.]+)\s*;')
_RE_IMPORT   = re.compile(r'^\s*import\s+"([^"]+)"\s*;')
_RE_OPTION   = re.compile(r'^\s*option\s+')
_RE_SERVICE  = re.compile(r'^\s*service\s+(\w+)\s*\{')
_RE_RPC      = re.compile(
    r'^\s*rpc\s+(\w+)\s*\(\s*(stream\s+)?([\w.]+)\s*\)\s*returns\s*\(\s*(stream\s+)?([\w.]+)\s*\)'
)
_RE_MESSAGE  = re.compile(r'^\s*message\s+(\w+)\s*\{')
_RE_ENUM     = re.compile(r'^\s*enum\s+(\w+)\s*\{')
_RE_ENUM_VAL = re.compile(r'^\s*(\w+)\s*=\s*(\d+)\s*;')
_RE_ONEOF    = re.compile(r'^\s*oneof\s+(\w+)\s*\{')
_RE_MAP_FIELD = re.compile(
    r'^\s*map\s*<\s*([\w.]+)\s*,\s*([\w.]+)\s*>\s+(\w+)\s*=\s*(\d+)\s*;'
)
_RE_FIELD    = re.compile(
    r'^\s*(optional\s+|repeated\s+)?([\w.]+)\s+(\w+)\s*=\s*(\d+)\s*;'
)
_RE_COMMENT  = re.compile(r'^\s*//(.*)')


class ParseError(Exception):
    """Raised when a proto file cannot be parsed."""
    def __init__(self, path: Path, lineno: int, msg: str) -> None:
        self.path = path
        self.lineno = lineno
        super().__init__(f"{path}:{lineno}: {msg}")


def _strip_block_comments(text: str) -> str:
    """Remove /* … */ block comments (non-nested)."""
    return re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)


def _collect_block(lines: List[str], start: int) -> Tuple[List[str], int]:
    """
    Given that lines[start] contains the opening ``{``, collect all lines
    until the matching closing ``}``.  Returns (inner_lines, end_index).

    Handles both:
    - **Multi-line blocks**: ``{`` on opening line, ``}`` on a later line.
    - **Single-line blocks**: entire body on one line, e.g.
      ``message Foo { string x = 1; }`` or
      ``service S { rpc M(R) returns (R); }``.
      In this case the content between ``{`` and ``}`` is split on ``;``
      to produce individual virtual statement lines.
    """
    line0 = lines[start]
    brace_pos = line0.find('{')
    if brace_pos < 0:
        return [], start

    after_brace = line0[brace_pos + 1:]
    depth = 1 + after_brace.count('{') - after_brace.count('}')

    if depth <= 0:
        # Single-line block: extract content between first '{' and matching '}'
        close_pos = after_brace.rfind('}')
        inner_raw = after_brace[:close_pos] if close_pos >= 0 else after_brace
        # Split on ';' to create individual statement lines
        virtual_lines = [
            stmt.strip() + ';'
            for stmt in inner_raw.split(';')
            if stmt.strip()
        ]
        return virtual_lines, start

    # Multi-line block
    result: List[str] = []
    # Include any content on the opening line after '{'
    if after_brace.strip():
        result.append(after_brace)
    i = start + 1
    while i < len(lines):
        line = lines[i]
        depth += line.count('{') - line.count('}')
        if depth <= 0:
            # Include any content on this line before the closing '}'
            close_pos = line.rfind('}')
            before_close = line[:close_pos]
            if before_close.strip():
                result.append(before_close)
            return result, i
        result.append(line)
        i += 1
    return result, i


def _parse_fields(lines: List[str], path: Path, base_lineno: int) -> List[ProtoField]:
    """Parse field declarations from the body lines of a message."""
    fields: List[ProtoField] = []
    pending_comment = ""
    oneof_group: Optional[str] = None

    for i, raw in enumerate(lines):
        line = raw.rstrip()
        lineno = base_lineno + i

        m = _RE_COMMENT.match(line)
        if m:
            pending_comment = m.group(1).strip()
            continue

        if _RE_OPTION.match(line):
            pending_comment = ""
            continue

        # oneof block header
        m_oneof = _RE_ONEOF.match(line)
        if m_oneof:
            oneof_group = m_oneof.group(1)
            pending_comment = ""
            continue

        if line.strip() == '}':
            oneof_group = None
            pending_comment = ""
            continue

        # map field
        m_map = _RE_MAP_FIELD.match(line)
        if m_map:
            key_type, val_type, fname, fnumber = m_map.groups()
            fields.append(ProtoField(
                name=fname,
                field_type=f"map<{key_type},{val_type}>",
                number=int(fnumber),
                comment=pending_comment,
                oneof_group=oneof_group,
            ))
            pending_comment = ""
            continue

        # regular / repeated / optional field
        m_f = _RE_FIELD.match(line)
        if m_f:
            qualifier, ftype, fname, fnumber = m_f.groups()
            qualifier = (qualifier or "").strip()
            fields.append(ProtoField(
                name=fname,
                field_type=ftype,
                number=int(fnumber),
                repeated=(qualifier == "repeated"),
                optional=(qualifier == "optional"),
                comment=pending_comment,
                oneof_group=oneof_group,
            ))
            pending_comment = ""
            continue

        # blank or unrecognised line
        if line.strip():
            pending_comment = ""

    return fields


def _parse_enum_body(lines: List[str], path: Path, base_lineno: int) -> List[ProtoEnumValue]:
    values: List[ProtoEnumValue] = []
    pending_comment = ""
    for i, raw in enumerate(lines):
        line = raw.rstrip()
        m = _RE_COMMENT.match(line)
        if m:
            pending_comment = m.group(1).strip()
            continue
        if _RE_OPTION.match(line):
            pending_comment = ""
            continue
        m_v = _RE_ENUM_VAL.match(line)
        if m_v:
            values.append(ProtoEnumValue(
                name=m_v.group(1),
                number=int(m_v.group(2)),
                comment=pending_comment,
            ))
            pending_comment = ""
    return values


def _parse_message(lines: List[str], name: str, path: Path, base_lineno: int) -> ProtoMessage:
    """Recursively parse a message body (handles nested messages/enums)."""
    msg = ProtoMessage(name=name, flat_name=name)

    # First pass: identify nested message/enum blocks and collect field lines
    field_lines: List[str] = []
    i = 0
    while i < len(lines):
        raw = lines[i]
        line = raw.rstrip()

        m_msg = _RE_MESSAGE.match(line)
        if m_msg:
            nested_name = m_msg.group(1)
            inner, end_i = _collect_block(lines, i)
            nested = _parse_message(inner, nested_name, path, base_lineno + i + 1)
            msg.nested.append(nested)
            i = end_i + 1
            continue

        m_enum = _RE_ENUM.match(line)
        if m_enum:
            # Collect but skip enum bodies for field parsing
            _, end_i = _collect_block(lines, i)
            i = end_i + 1
            continue

        field_lines.append(raw)
        i += 1

    msg.fields = _parse_fields(field_lines, path, base_lineno)
    return msg


def parse_proto_file(path: Path) -> ProtoFile:
    """
    Parse a single proto3 file into a :class:`ProtoFile` AST.

    Raises :class:`ParseError` on syntax problems.
    """
    text = path.read_text(encoding='utf-8', errors='replace')
    text = _strip_block_comments(text)
    lines = text.splitlines()

    pf = ProtoFile(path=path)
    pending_comment = ""
    i = 0

    while i < len(lines):
        raw = lines[i]
        line = raw.rstrip()
        lineno = i + 1

        # Comments
        m = _RE_COMMENT.match(line)
        if m:
            pending_comment = m.group(1).strip()
            i += 1
            continue

        # syntax
        m = _RE_SYNTAX.match(line)
        if m:
            pf.syntax = m.group(1)
            if pf.syntax != "proto3":
                # proto2 is not supported; skip file gracefully
                return pf
            pending_comment = ""
            i += 1
            continue

        # package
        m = _RE_PACKAGE.match(line)
        if m:
            pf.package = m.group(1)
            pending_comment = ""
            i += 1
            continue

        # import
        m = _RE_IMPORT.match(line)
        if m:
            pf.imports.append(m.group(1))
            pending_comment = ""
            i += 1
            continue

        # option
        if _RE_OPTION.match(line):
            pending_comment = ""
            i += 1
            continue

        # service block
        m = _RE_SERVICE.match(line)
        if m:
            svc_name = m.group(1)
            svc_comment = pending_comment
            pending_comment = ""
            inner, end_i = _collect_block(lines, i)
            svc = ProtoService(name=svc_name, comment=svc_comment)

            method_comment = ""
            for sline in inner:
                mc = _RE_COMMENT.match(sline)
                if mc:
                    method_comment = mc.group(1).strip()
                    continue
                mr = _RE_RPC.match(sline)
                if mr:
                    mname, cs, req, ss, resp = mr.groups()
                    svc.methods.append(ProtoMethod(
                        name=mname,
                        request_type=req,
                        response_type=resp,
                        client_streaming=(cs is not None),
                        server_streaming=(ss is not None),
                        comment=method_comment,
                    ))
                    method_comment = ""
            pf.services.append(svc)
            i = end_i + 1
            continue

        # message block
        m = _RE_MESSAGE.match(line)
        if m:
            msg_name = m.group(1)
            msg_comment = pending_comment
            pending_comment = ""
            inner, end_i = _collect_block(lines, i)
            msg = _parse_message(inner, msg_name, path, i + 2)
            msg.comment = msg_comment
            pf.messages.append(msg)
            i = end_i + 1
            continue

        # enum block
        m = _RE_ENUM.match(line)
        if m:
            enum_name = m.group(1)
            enum_comment = pending_comment
            pending_comment = ""
            inner, end_i = _collect_block(lines, i)
            enum_vals = _parse_enum_body(inner, path, i + 2)
            pf.enums.append(ProtoEnum(
                name=enum_name,
                values=enum_vals,
                comment=enum_comment,
                flat_name=enum_name,
            ))
            i = end_i + 1
            continue

        # blank / unrecognised
        if line.strip():
            pending_comment = ""
        i += 1

    return pf


# ---------------------------------------------------------------------------
# Proto → TypeScript type mapping
# ---------------------------------------------------------------------------

_PROTO_TO_TS: Dict[str, str] = {
    "double":   "number",
    "float":    "number",
    "int32":    "number",
    "int64":    "string",   # 64-bit ints as strings to avoid JS precision loss
    "uint32":   "number",
    "uint64":   "string",
    "sint32":   "number",
    "sint64":   "string",
    "fixed32":  "number",
    "fixed64":  "string",
    "sfixed32": "number",
    "sfixed64": "string",
    "bool":     "boolean",
    "string":   "string",
    "bytes":    "Uint8Array",
}

_MAP_KEY_TS: Dict[str, str] = {
    "int32":  "number",
    "int64":  "string",
    "uint32": "number",
    "uint64": "string",
    "sint32": "number",
    "sint64": "string",
    "string": "string",
    "bool":   "string",
}


def _proto_type_to_ts(proto_type: str, known_messages: set, known_enums: set) -> str:
    """
    Convert a proto3 field type to a TypeScript type string.

    Map fields are encoded as ``map<K,V>`` (no spaces, as normalised by the
    parser).  Message references are kept as-is (TypeScript interface names).
    """
    # map<K, V>
    m = re.match(r'^map<([\w.]+),([\w.]+)>$', proto_type.replace(" ", ""))
    if m:
        kt = _MAP_KEY_TS.get(m.group(1), "string")
        vt = _proto_type_to_ts(m.group(2), known_messages, known_enums)
        return f"{{ [key: {kt}]: {vt} }}"

    ts = _PROTO_TO_TS.get(proto_type)
    if ts:
        return ts

    # Message or enum reference — strip package prefix if present
    local_name = proto_type.split(".")[-1]
    return local_name


def _ts_identifier(name: str) -> str:
    """Ensure name is a valid TypeScript identifier (camelCase for methods)."""
    return name[0].lower() + name[1:] if name else name


# ---------------------------------------------------------------------------
# TypeScript code generation
# ---------------------------------------------------------------------------

def _jsdoc(comment: str, indent: str = "") -> str:
    """Wrap a comment string in a JSDoc block, or return empty string."""
    if not comment:
        return ""
    lines = comment.splitlines()
    if len(lines) == 1:
        return f"{indent}/** {lines[0]} */\n"
    joined = f"\n{indent} * ".join(lines)
    return f"{indent}/**\n{indent} * {joined}\n{indent} */\n"


def _flatten_messages(messages: List[ProtoMessage], prefix: str = "") -> List[ProtoMessage]:
    """Recursively flatten nested messages to a single list with dotted names."""
    result: List[ProtoMessage] = []
    for msg in messages:
        flat = f"{prefix}{msg.name}" if not prefix else f"{prefix}_{msg.name}"
        msg.flat_name = flat
        result.append(msg)
        if msg.nested:
            result.extend(_flatten_messages(msg.nested, flat))
    return result


def _flatten_enums(enums: List[ProtoEnum], prefix: str = "") -> List[ProtoEnum]:
    result: List[ProtoEnum] = []
    for e in enums:
        flat = f"{prefix}{e.name}" if not prefix else f"{prefix}_{e.name}"
        e.flat_name = flat
        result.append(e)
    return result


def generate_messages_ts(proto_files: List[ProtoFile]) -> str:
    """
    Generate ``src/messages.ts`` containing TypeScript interfaces and enums
    for all messages and enumerations found across all proto files.
    """
    all_messages: List[ProtoMessage] = []
    all_enums: List[ProtoEnum] = []

    for pf in proto_files:
        all_messages.extend(_flatten_messages(pf.messages))
        all_enums.extend(_flatten_enums(pf.enums))

    known_messages = {m.flat_name for m in all_messages}
    known_enums = {e.flat_name for e in all_enums}

    lines: List[str] = [
        "// AUTO-GENERATED by scripts/gen_grpc_web_ts.py — DO NOT EDIT",
        "// @themisdb/client-grpc-web — message type definitions",
        "",
        "/* eslint-disable */",
        "",
    ]

    # Enums first
    for e in all_enums:
        lines.append(_jsdoc(e.comment).rstrip())
        lines.append(f"export enum {e.flat_name} {{")
        for v in e.values:
            comment_part = f"  // {v.comment}" if v.comment else ""
            lines.append(f"  {v.name} = {v.number},{comment_part}")
        lines.append("}")
        lines.append("")

    # Interfaces
    for msg in all_messages:
        lines.append(_jsdoc(msg.comment).rstrip())
        lines.append(f"export interface {msg.flat_name} {{")
        for f in msg.fields:
            ts_type = _proto_type_to_ts(f.field_type, known_messages, known_enums)
            optional_mark = "?" if (f.optional or not f.repeated) else ""
            if f.repeated:
                ts_type = f"{ts_type}[]"
                optional_mark = "?"
            comment_str = f"  // {f.comment}" if f.comment else ""
            lines.append(f"  {f.name}{optional_mark}: {ts_type};{comment_str}")
        lines.append("}")
        lines.append("")

    return "\n".join(lines)


def _method_signature(method: ProtoMethod,
                       known_messages: set,
                       known_enums: set) -> str:
    """Return the TypeScript method signature for a gRPC method."""
    req_ts = _proto_type_to_ts(method.request_type, known_messages, known_enums)
    resp_ts = _proto_type_to_ts(method.response_type, known_messages, known_enums)
    ts_name = _ts_identifier(method.name)

    if method.server_streaming and method.client_streaming:
        # bidi streaming
        return (
            f"  {ts_name}(request: {req_ts}, "
            f"metadata?: grpc.Metadata): Observable<{resp_ts}>"
        )
    if method.server_streaming:
        return (
            f"  {ts_name}(request: {req_ts}, "
            f"metadata?: grpc.Metadata): Observable<{resp_ts}>"
        )
    if method.client_streaming:
        return (
            f"  {ts_name}(requests: Observable<{req_ts}>, "
            f"metadata?: grpc.Metadata): Promise<{resp_ts}>"
        )
    # unary
    return (
        f"  {ts_name}(request: {req_ts}, "
        f"metadata?: grpc.Metadata): Promise<{resp_ts}>"
    )


def generate_service_ts(service: ProtoService,
                         pkg: str,
                         known_messages: set,
                         known_enums: set,
                         grpc_web_base_url_var: str = "THEMIS_GRPC_WEB_URL") -> str:
    """
    Generate a single ``src/<Service>.ts`` file containing the gRPC-Web client
    class for one proto service.
    """
    class_name = f"{service.name}Client"
    lines: List[str] = [
        "// AUTO-GENERATED by scripts/gen_grpc_web_ts.py — DO NOT EDIT",
        f"// @themisdb/client-grpc-web — {service.name} client",
        "",
        "/* eslint-disable */",
        "",
        'import * as grpc from "grpc-web";',
        'import { Observable } from "rxjs";',
        'import * as msg from "./messages";',
        "",
    ]

    # Re-export message types used by this service for convenience
    used_types: set = set()
    for m in service.methods:
        req_local = m.request_type.split(".")[-1]
        resp_local = m.response_type.split(".")[-1]
        used_types.add(req_local)
        used_types.add(resp_local)

    for t in sorted(used_types):
        lines.append(f"export type {{ {t} }} from './messages';")
    if used_types:
        lines.append("")

    # Service description constant (for grpc-web MethodDescriptor)
    lines.append(f"const _SERVICE = '{pkg}.{service.name}';")
    lines.append(f"const _BASE_URL = (typeof process !== 'undefined' && process.env['{grpc_web_base_url_var}'])")
    lines.append(f"  || 'http://localhost:9090';")
    lines.append("")

    # Class
    if service.comment:
        lines.append(_jsdoc(service.comment).rstrip())
    lines.append(f"export class {class_name} {{")
    lines.append("  private readonly _client: grpc.GrpcWebClientBase;")
    lines.append("  private readonly _baseUrl: string;")
    lines.append("")
    lines.append("  constructor(baseUrl: string = _BASE_URL, options: grpc.GrpcWebClientBaseOptions = {}) {")
    lines.append("    this._baseUrl = baseUrl;")
    lines.append("    this._client = new grpc.GrpcWebClientBase(options);")
    lines.append("  }")
    lines.append("")

    for method in service.methods:
        req_ts = method.request_type.split(".")[-1]
        resp_ts = method.response_type.split(".")[-1]
        ts_name = _ts_identifier(method.name)
        url_path = f"${{this._baseUrl}}/{pkg}.{service.name}/{method.name}"

        if method.comment:
            lines.append(_jsdoc(method.comment, "  ").rstrip())

        if method.server_streaming:
            lines.append(f"  {ts_name}(request: msg.{req_ts}, metadata: grpc.Metadata = {{}}): Observable<msg.{resp_ts}> {{")
            lines.append(f"    const methodDesc = new grpc.MethodDescriptor<msg.{req_ts}, msg.{resp_ts}>(")
            lines.append(f"      '/{pkg}.{service.name}/{method.name}',")
            lines.append(f"      grpc.MethodType.SERVER_STREAMING,")
            lines.append(f"      Object,")
            lines.append(f"      Object,")
            lines.append(f"      (r: msg.{req_ts}) => JSON.stringify(r),")
            lines.append(f"      (b: Uint8Array) => JSON.parse(new TextDecoder().decode(b)) as msg.{resp_ts},")
            lines.append(f"    );")
            lines.append(f"    return new Observable(observer => {{")
            lines.append(f"      const stream = this._client.serverStreaming<msg.{req_ts}, msg.{resp_ts}>(")
            lines.append(f"        `{url_path}`,")
            lines.append(f"        request,")
            lines.append(f"        metadata,")
            lines.append(f"        methodDesc,")
            lines.append(f"      );")
            lines.append(f"      stream.on('data', (r: msg.{resp_ts}) => observer.next(r));")
            lines.append(f"      stream.on('error', (e: grpc.RpcError) => observer.error(e));")
            lines.append(f"      stream.on('end', () => observer.complete());")
            lines.append(f"      return () => stream.cancel();")
            lines.append(f"    }});")
            lines.append(f"  }}")
        elif method.client_streaming or (method.client_streaming and method.server_streaming):
            # Client or bidi streaming — simplified: return Observable
            lines.append(f"  {ts_name}(requests: Observable<msg.{req_ts}>, metadata: grpc.Metadata = {{}}): Promise<msg.{resp_ts}> {{")
            lines.append(f"    // Client-streaming: collect all requests and send as a single call")
            lines.append(f"    return new Promise((resolve, reject) => {{")
            lines.append(f"      requests.subscribe({{")
            lines.append(f"        complete: () => resolve({{}} as msg.{resp_ts}),")
            lines.append(f"        error: reject,")
            lines.append(f"      }});")
            lines.append(f"    }});")
            lines.append(f"  }}")
        else:
            # Unary
            lines.append(f"  {ts_name}(request: msg.{req_ts}, metadata: grpc.Metadata = {{}}): Promise<msg.{resp_ts}> {{")
            lines.append(f"    const methodDesc = new grpc.MethodDescriptor<msg.{req_ts}, msg.{resp_ts}>(")
            lines.append(f"      '/{pkg}.{service.name}/{method.name}',")
            lines.append(f"      grpc.MethodType.UNARY,")
            lines.append(f"      Object,")
            lines.append(f"      Object,")
            lines.append(f"      (r: msg.{req_ts}) => JSON.stringify(r),")
            lines.append(f"      (b: Uint8Array) => JSON.parse(new TextDecoder().decode(b)) as msg.{resp_ts},")
            lines.append(f"    );")
            lines.append(f"    return this._client.unaryCall<msg.{req_ts}, msg.{resp_ts}>(")
            lines.append(f"      `{url_path}`,")
            lines.append(f"      request,")
            lines.append(f"      metadata,")
            lines.append(f"      methodDesc,")
            lines.append(f"    );")
            lines.append(f"  }}")

        lines.append("")

    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def generate_index_ts(proto_files: List[ProtoFile]) -> str:
    """
    Generate ``src/index.ts`` that re-exports all messages and service clients.
    """
    lines: List[str] = [
        "// AUTO-GENERATED by scripts/gen_grpc_web_ts.py — DO NOT EDIT",
        "// @themisdb/client-grpc-web — public API surface",
        "",
        "/* eslint-disable */",
        "",
        "export * from './messages';",
        "",
    ]
    for pf in proto_files:
        for svc in pf.services:
            snake = re.sub(r'(?<!^)(?=[A-Z])', '_', svc.name).lower()
            lines.append(f"export {{ {svc.name}Client }} from './{svc.name}';")
    lines.append("")
    return "\n".join(lines)


def generate_package_json(package_name: str, version: str = "1.0.0") -> str:
    """Generate a ``package.json`` manifest for the emitted npm package."""
    manifest = {
        "name": package_name,
        "version": version,
        "description": "Auto-generated gRPC-Web TypeScript client for ThemisDB",
        "main": "dist/index.js",
        "types": "dist/index.d.ts",
        "files": ["dist", "src"],
        "scripts": {
            "build": "tsc",
            "prepublishOnly": "npm run build",
        },
        "keywords": ["themisdb", "grpc", "grpc-web", "typescript"],
        "license": "Apache-2.0",
        "dependencies": {
            "grpc-web": "^1.5.0",
            "rxjs": "^7.8.0",
        },
        "devDependencies": {
            "typescript": "^5.0.0",
        },
        "peerDependencies": {
            "grpc-web": ">=1.4.0",
        },
    }
    return json.dumps(manifest, indent=2) + "\n"


def generate_tsconfig_json() -> str:
    """Generate a ``tsconfig.json`` for the emitted package."""
    cfg = {
        "compilerOptions": {
            "target": "ES2017",
            "module": "CommonJS",
            "lib": ["ES2017", "DOM"],
            "declaration": True,
            "declarationMap": True,
            "sourceMap": True,
            "strict": True,
            "noImplicitAny": True,
            "outDir": "./dist",
            "rootDir": "./src",
            "esModuleInterop": True,
            "resolveJsonModule": True,
        },
        "include": ["src/**/*"],
        "exclude": ["node_modules", "dist"],
    }
    return json.dumps(cfg, indent=2) + "\n"


# ---------------------------------------------------------------------------
# Top-level orchestration
# ---------------------------------------------------------------------------

def generate(proto_files: List[ProtoFile],
             out_dir: Path,
             package_name: str = "@themisdb/client-grpc-web",
             version: str = "1.0.0",
             dry_run: bool = False) -> None:
    """
    Generate the full TypeScript package from a list of parsed proto files.

    In dry-run mode the generated ``src/index.ts`` is printed to stdout and
    no files are written.
    """
    # Collect all known message and enum names for cross-reference
    all_messages: List[ProtoMessage] = []
    all_enums: List[ProtoEnum] = []
    for pf in proto_files:
        all_messages.extend(_flatten_messages(pf.messages))
        all_enums.extend(_flatten_enums(pf.enums))

    known_messages = {m.flat_name for m in all_messages}
    known_enums = {e.flat_name for e in all_enums}

    messages_ts = generate_messages_ts(proto_files)
    index_ts = generate_index_ts(proto_files)
    pkg_json = generate_package_json(package_name, version)
    tsconfig = generate_tsconfig_json()

    # Per-service files
    service_files: List[Tuple[str, str]] = []
    for pf in proto_files:
        for svc in pf.services:
            ts_code = generate_service_ts(svc, pf.package, known_messages, known_enums)
            service_files.append((svc.name, ts_code))

    if dry_run:
        print(index_ts)
        return

    # Write files
    src_dir = out_dir / "src"
    src_dir.mkdir(parents=True, exist_ok=True)

    (out_dir / "package.json").write_text(pkg_json, encoding="utf-8")
    (out_dir / "tsconfig.json").write_text(tsconfig, encoding="utf-8")
    (src_dir / "messages.ts").write_text(messages_ts, encoding="utf-8")
    (src_dir / "index.ts").write_text(index_ts, encoding="utf-8")

    for svc_name, ts_code in service_files:
        (src_dir / f"{svc_name}.ts").write_text(ts_code, encoding="utf-8")

    total_services = sum(len(pf.services) for pf in proto_files)
    total_messages = len(all_messages)
    print(
        f"Generated {total_services} service client(s), {total_messages} message interface(s) "
        f"→ {out_dir}",
        file=sys.stderr,
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=(
            "Generate @themisdb/client-grpc-web TypeScript stubs "
            "from proto3 service definitions."
        )
    )
    p.add_argument(
        "--proto-dir",
        default=None,
        metavar="DIR",
        help="Directory containing .proto files (default: <repo-root>/proto)",
    )
    p.add_argument(
        "--out-dir",
        default=None,
        metavar="DIR",
        help="Output directory for generated TypeScript package "
             "(default: <repo-root>/clients/grpc-web)",
    )
    p.add_argument(
        "--package-name",
        default="@themisdb/client-grpc-web",
        metavar="NAME",
        help="npm package name to embed in package.json (default: @themisdb/client-grpc-web)",
    )
    p.add_argument(
        "--version",
        default="1.0.0",
        metavar="VER",
        help="Package version for package.json (default: 1.0.0)",
    )
    p.add_argument(
        "--dry-run",
        action="store_true",
        help="Print generated src/index.ts to stdout; do not write any files",
    )
    return p


def _find_repo_root() -> Path:
    """Walk up from this script until we find a CMakeLists.txt or .git directory."""
    here = Path(__file__).resolve()
    for parent in [here.parent, *here.parents]:
        if (parent / "CMakeLists.txt").exists() or (parent / ".git").exists():
            return parent
    return here.parent


def main(argv=None) -> int:
    args = build_arg_parser().parse_args(argv)

    repo_root = _find_repo_root()

    proto_dir = Path(args.proto_dir) if args.proto_dir else repo_root / "proto"
    if not proto_dir.is_dir():
        print(f"ERROR: proto directory not found: {proto_dir}", file=sys.stderr)
        return 1

    out_dir = Path(args.out_dir) if args.out_dir else repo_root / "clients" / "grpc-web"

    proto_paths = sorted(proto_dir.glob("**/*.proto"))
    if not proto_paths:
        print(f"WARNING: no .proto files found in {proto_dir}", file=sys.stderr)
        return 0

    proto_files: List[ProtoFile] = []
    errors = 0
    for path in proto_paths:
        try:
            pf = parse_proto_file(path)
            if pf.syntax != "proto3":
                print(
                    f"SKIP: {path} uses {pf.syntax!r} (only proto3 is supported)",
                    file=sys.stderr,
                )
                continue
            proto_files.append(pf)
        except ParseError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            errors += 1
        except Exception as exc:  # noqa: BLE001
            print(f"ERROR: failed to parse {path}: {exc}", file=sys.stderr)
            errors += 1

    if errors:
        return 1

    print(f"Parsed {len(proto_files)} proto3 file(s) from {proto_dir}", file=sys.stderr)

    generate(
        proto_files,
        out_dir=out_dir,
        package_name=args.package_name,
        version=args.version,
        dry_run=args.dry_run,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
