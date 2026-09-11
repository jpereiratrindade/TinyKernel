#!/usr/bin/env python3
"""
TinyKernel Web Server (SisTer Interface)
Lightweight HTTP server with auto-port finding and clean logging.
"""

import argparse
import http.server
import json
import os
import pathlib
import socketserver
import sqlite3
import subprocess
import sys
import tempfile
import threading
import urllib.parse
import webbrowser

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = pathlib.Path(DIRECTORY).parent


class CoreBridge:
    """Read-through bridge from the web product to the canonical C++ engine."""

    def __init__(self, workspace):
        self.workspace = pathlib.Path(workspace).resolve()
        self.cli = REPO_ROOT / "build" / "bin" / "tinykernel"
        self._lock = threading.Lock()

    def _run(self, *arguments, input_text=None):
        command = [str(self.cli), "--workspace", str(self.workspace), *arguments]
        with self._lock:
            completed = self._execute(command, input_text=input_text)
        if completed.returncode != 0:
            raise RuntimeError(completed.stderr.strip() or completed.stdout.strip() or "falha no núcleo TinyKernel")
        return completed.stdout

    def _execute(self, command, input_text=None):
        # The browser loads cards concurrently. Each CLI invocation initializes
        # the SQLite repository, so callers serialize processes for one workspace.
        return subprocess.run(command, cwd=REPO_ROOT, text=True, input=input_text,
                              capture_output=True, check=False)

    def initialize(self):
        self.workspace.mkdir(parents=True, exist_ok=True)
        if not self.cli.exists():
            completed = subprocess.run(
                [str(REPO_ROOT / "bin" / "tinykernel"), "version"],
                cwd=REPO_ROOT, text=True, capture_output=True, check=False
            )
            if completed.returncode != 0:
                raise RuntimeError(completed.stderr.strip() or "não foi possível construir o núcleo")
        database = self.workspace / "tinykernel.sqlite3"
        if not database.exists():
            self._run("init", str(self.workspace))
            self._run("run", "TK-0000")
            self._run("run", "TK-0001")

    def list_studies(self):
        return json.loads(self._run("list", "--json"))

    def get_study(self, study_id):
        return json.loads(self._run("show", study_id, "--json"))

    def export_study(self, study_id):
        return self._run("export", study_id).encode("utf-8")

    def project_study(self, study_id):
        return json.loads(self._run("project", study_id))

    def export_workspace(self):
        database = self.workspace / "tinykernel.sqlite3"
        with self._lock:
            if not database.exists():
                raise RuntimeError("workspace não inicializado")
            with tempfile.TemporaryDirectory(prefix="tinykernel-export-", dir=self.workspace.parent) as folder:
                snapshot = pathlib.Path(folder) / "tinykernel.sqlite3"
                source = sqlite3.connect(f"file:{database}?mode=ro", uri=True)
                target = sqlite3.connect(snapshot)
                try:
                    source.backup(target)
                finally:
                    target.close()
                    source.close()
                return snapshot.read_bytes()

    def import_workspace(self, content):
        if not content.startswith(b"SQLite format 3\x00"):
            raise ValueError("arquivo não é um workspace SQLite do TinyKernel")
        self.workspace.parent.mkdir(parents=True, exist_ok=True)
        with self._lock, tempfile.TemporaryDirectory(prefix="tinykernel-import-", dir=self.workspace.parent) as folder:
            candidate = pathlib.Path(folder) / "tinykernel.sqlite3"
            candidate.write_bytes(content)
            command = [str(self.cli), "--workspace", folder, "integrity"]
            completed = self._execute(command)
            if completed.returncode != 0:
                raise ValueError(completed.stderr.strip() or "workspace reprovado na verificação de integridade")
            for suffix in ("-wal", "-shm"):
                sidecar = pathlib.Path(f"{self.workspace / 'tinykernel.sqlite3'}{suffix}")
                if sidecar.exists():
                    sidecar.unlink()
            os.replace(candidate, self.workspace / "tinykernel.sqlite3")
        return self.list_studies()

    def run_study(self, study_id):
        self._run("run", study_id, "--json")
        return self.get_study(study_id)

    def _mutation(self, command, study_id=None, fields=None):
        arguments = [command]
        if study_id is not None:
            arguments.append(study_id)
        encoded = urllib.parse.urlencode(fields or {}, doseq=True)
        return json.loads(self._run(*arguments, input_text=encoded))

    def create_study(self, study):
        investigation = study.get("investigation") or {}
        phenomenon = study.get("phenomenon") or {}
        context = study.get("context") or {}
        profile = study.get("constitutive_profile") or {}
        realizations = study.get("realizations") or []
        baseline = realizations[0] if realizations else {}
        interventions = study.get("interventions") or []
        fields = {
            "id": investigation.get("id", ""),
            "title": investigation.get("title") or phenomenon.get("name", ""),
            "phenomenon_description": phenomenon.get("description") or phenomenon.get("definition", ""),
            "context_description": context.get("description", ""),
            "baseline_label": baseline.get("label", ""),
            "component": baseline.get("components", []),
            "distinction": profile.get("dimensions") or profile.get("distinctions", []),
            "relation": profile.get("essential_relations") or profile.get("relations", []),
            "temporal_constraint": profile.get("temporal_bounds") or profile.get("temporal_constraints", []),
            "intervention_kind": [item.get("kind", "remove") for item in interventions],
            "intervention_target": [item.get("target_component", "") for item in interventions],
            "intervention_replacement": [item.get("replacement_component", "") for item in interventions],
        }
        return self._mutation("create", fields=fields)

    def materialize(self, study_id, payload):
        return self._mutation("materialize", study_id, {
            "planned_id": payload.get("planned_id", ""),
            "source_id": payload.get("source", ""),
            "kind": payload.get("kind", ""),
            "target": payload.get("target_component", ""),
            "replacement": payload.get("replacement_component", ""),
            "protocol": payload.get("protocol", ""),
        })

    def observe(self, study_id, payload):
        return self._mutation("observe", study_id, {
            "realization_id": payload.get("realization_id", ""),
            "dimension": payload.get("dimension", ""),
            "satisfied": "true" if payload.get("satisfied", False) else "false",
            "trace": payload.get("trace", ""),
        })

    def adjudicate(self, study_id):
        return self._mutation("adjudicate", study_id)

    def infer(self, study_id):
        return self._mutation("infer", study_id)

    def delete(self, study_id):
        return self._mutation("delete", study_id)


class Handler(http.server.SimpleHTTPRequestHandler):
    bridge = None

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def _json_response(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _api_route(self):
        path = urllib.parse.urlparse(self.path).path
        if path == "/api/health":
            return "health", None
        if path == "/api/workspace/export":
            return "workspace_export", None
        if path == "/api/workspace/import":
            return "workspace_import", None
        if path == "/api/studies":
            return "list", None
        prefix = "/api/studies/"
        if path.startswith(prefix):
            remainder = urllib.parse.unquote(path[len(prefix):])
            actions = {
                "/run": "run",
                "/interventions": "materialize",
                "/observations": "observe",
                "/adjudicate": "adjudicate",
                "/infer": "infer",
                "/export": "export",
                "/analysis": "analysis",
            }
            for suffix, action in actions.items():
                if remainder.endswith(suffix):
                    return action, remainder[:-len(suffix)].rstrip("/")
            if remainder and "/" not in remainder:
                return "get", remainder
        return None, None

    def do_GET(self):
        action, study_id = self._api_route()
        if action:
            try:
                if action == "health":
                    payload = {"status": "ready", "source": "libtinykernel", "workspace": str(self.bridge.workspace)}
                elif action == "list":
                    payload = {"studies": self.bridge.list_studies(), "source": "libtinykernel"}
                elif action == "get":
                    payload = self.bridge.get_study(study_id)
                    payload["runtime_source"] = "libtinykernel"
                    payload["workflow_projection"] = self.bridge.project_study(study_id)
                elif action == "analysis":
                    payload = self.bridge.project_study(study_id)
                elif action == "export":
                    return self._binary_response(200, self.bridge.export_study(study_id),
                        "application/json; charset=utf-8", f'{study_id}.json')
                elif action == "workspace_export":
                    return self._binary_response(200, self.bridge.export_workspace(),
                        "application/vnd.sqlite3", "tinykernel-workspace.sqlite3")
                else:
                    return self._json_response(405, {"error": "use POST para executar uma investigação"})
                return self._json_response(200, payload)
            except Exception as error:
                return self._json_response(404 if action == "get" else 500, {"error": str(error)})
        return super().do_GET()

    def do_POST(self):
        action, study_id = self._api_route()
        if action == "workspace_import":
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 100_000_000:
                    raise ValueError("workspace ausente ou maior que 100 MB")
                studies = self.bridge.import_workspace(self.rfile.read(length))
                return self._json_response(200, {"studies": studies, "source": "libtinykernel"})
            except Exception as error:
                return self._json_response(400, {"error": str(error)})
        if action not in {"list", "run", "materialize", "observe", "adjudicate", "infer"}:
            return self._json_response(404, {"error": "endpoint não encontrado"})
        try:
            request = self._request_json() if int(self.headers.get("Content-Length", "0")) > 0 else {}
            if action == "list":
                payload = self.bridge.create_study(request)
            elif action == "run":
                payload = self.bridge.run_study(study_id)
            elif action == "materialize":
                payload = self.bridge.materialize(study_id, request)
            elif action == "observe":
                payload = self.bridge.observe(study_id, request)
            elif action == "adjudicate":
                payload = self.bridge.adjudicate(study_id)
            else:
                payload = self.bridge.infer(study_id)
            payload["runtime_source"] = "libtinykernel"
            projection_id = study_id or payload.get("investigation", {}).get("id")
            payload["workflow_projection"] = self.bridge.project_study(projection_id)
            return self._json_response(201 if action == "list" else 200, payload)
        except Exception as error:
            return self._json_response(400, {"error": str(error)})

    def do_DELETE(self):
        action, study_id = self._api_route()
        if action != "get":
            return self._json_response(404, {"error": "endpoint não encontrado"})
        try:
            return self._json_response(200, self.bridge.delete(study_id))
        except Exception as error:
            return self._json_response(400, {"error": str(error)})

    def _request_json(self):
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0 or length > 2_000_000:
            raise ValueError("corpo JSON ausente ou maior que 2 MB")
        value = json.loads(self.rfile.read(length).decode("utf-8"))
        if not isinstance(value, dict):
            raise ValueError("o corpo JSON deve ser um objeto")
        return value

    def _binary_response(self, status, body, content_type, filename):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Disposition", f'attachment; filename="{filename}"')
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def end_headers(self):
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

def run(port=PORT, open_browser=False, workspace=None, host="127.0.0.1"):
    bridge = CoreBridge(workspace or (REPO_ROOT / ".tinykernel-web"))
    bridge.initialize()
    Handler.bridge = bridge
    max_attempts = 10
    current_port = port
    
    for _ in range(max_attempts):
        try:
            with socketserver.ThreadingTCPServer((host, current_port), Handler) as httpd:
                httpd.daemon_threads = True
                url = f"http://localhost:{current_port}"
                print("=" * 60)
                print("  TinyKernel Web Interface (SisTer Ecosystem)")
                print("  Status: READY ≠ COMPLETE")
                print(f"  URL:    {url}")
                print("=" * 60)
                print("Pressione Ctrl+C para encerrar o servidor.")
                
                if open_browser:
                    webbrowser.open(url)
                
                httpd.serve_forever()
                return
        except OSError:
            current_port += 1

    print(f"Erro: Não foi possível vincular as portas {port}..{current_port-1}", file=sys.stderr)
    sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TinyKernel Web — Mesa de Investigação")
    parser.add_argument("port", nargs="?", type=int, default=PORT)
    parser.add_argument("--open", action="store_true", dest="open_browser")
    parser.add_argument("--workspace", default=str(REPO_ROOT / ".tinykernel-web"))
    parser.add_argument("--host", default="127.0.0.1")
    arguments = parser.parse_args()
    try:
        run(port=arguments.port, open_browser=arguments.open_browser,
            workspace=arguments.workspace, host=arguments.host)
    except KeyboardInterrupt:
        print("\nServidor TinyKernel encerrado.")
