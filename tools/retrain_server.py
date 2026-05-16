import argparse
import json
import subprocess
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Optional
from urllib.parse import urlparse


class RetrainJobState:
    def __init__(self, repo_root: Path, python_exe: str, default_input: str):
        self.repo_root = repo_root
        self.python_exe = python_exe
        self.default_input = default_input
        self.lock = threading.Lock()
        self.process = None
        self.last_status = {
            "status": "idle",
            "current_step": "idle",
            "raw_input": default_input,
            "status_path": str((repo_root / "artifacts" / "retrain" / "job_status.json").resolve()),
        }

    def status_path(self) -> Path:
        return Path(self.last_status["status_path"])

    def load_status(self):
        path = self.status_path()
        if path.exists():
            try:
                self.last_status = json.loads(path.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                pass
        return self.last_status

    def is_running(self) -> bool:
        return self.process is not None and self.process.poll() is None

    def start(self, raw_input: Optional[str] = None):
        with self.lock:
            self.load_status()
            if self.is_running():
                return False, self.last_status

            dataset = raw_input or self.default_input
            command = [
                self.python_exe,
                str(self.repo_root / "tools" / "run_retrain.py"),
                "--raw-input",
                dataset,
                "--baseline-csv",
            ]
            self.process = subprocess.Popen(
                command,
                cwd=str(self.repo_root),
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            self.last_status = {
                "status": "queued",
                "current_step": "spawned",
                "raw_input": dataset,
                "status_path": str((self.repo_root / "artifacts" / "retrain" / "job_status.json").resolve()),
            }
            return True, self.last_status


class RetrainHandler(BaseHTTPRequestHandler):
    state: RetrainJobState = None

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def send_json(self, payload, code=200):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self.send_response(204)
        self.end_headers()

    def do_GET(self):
        path = urlparse(self.path).path
        if path == "/health":
            self.send_json({"status": "ok"})
            return
        if path == "/status":
            self.send_json(self.state.load_status())
            return
        self.send_json({"error": "not_found"}, code=404)

    def do_POST(self):
        path = urlparse(self.path).path
        if path != "/start":
            self.send_json({"error": "not_found"}, code=404)
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        raw_body = self.rfile.read(content_length) if content_length else b"{}"
        try:
            payload = json.loads(raw_body.decode("utf-8"))
        except json.JSONDecodeError:
            self.send_json({"status": "error", "detail": "invalid_json"}, code=400)
            return

        raw_input = payload.get("raw_input")
        started, status = self.state.start(raw_input)
        if started:
            self.send_json({"status": "accepted", "detail": "worker_started", "job": status}, code=202)
        else:
            self.send_json({"status": "busy", "detail": "worker_running", "job": status}, code=409)


def main():
    parser = argparse.ArgumentParser(description="Local HTTP trigger for TinyML retrain pipeline")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument(
        "--default-input",
        default=r"C:\Users\tam\Documents\GitHub\TinyML_ESP32\sensor_data.csv",
        help="Default dataset path used when POST /start omits raw_input",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    state = RetrainJobState(repo_root, sys.executable, args.default_input)
    RetrainHandler.state = state

    server = ThreadingHTTPServer((args.host, args.port), RetrainHandler)
    print(f"Retrain server listening on http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
