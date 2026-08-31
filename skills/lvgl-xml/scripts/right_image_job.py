#!/usr/bin/env python3
"""Submit and collect Right image jobs for Magic Watch visual drafts.

The command is intentionally safe by default: without --confirm-paid it only
prints the request summary. API keys are read from the environment or a local
.env.local file and are never written to the manifest.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

SUBMIT_URL = "https://www.rightapi.ai/draw/v1/images/generations"
TASK_URL = "https://www.rightapi.ai/v1/tasks/{task_id}"
DEFAULT_MODEL = "gpt-image-2"
DEFAULT_SIZE = "9:16"
DEFAULT_IMAGE_SIZE = "1K"


def _load_local_env(path: Path) -> None:
    """Load simple KEY=VALUE pairs without overriding the process environment."""
    if not path.is_file():
        return
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip("\"'")
        if key and key not in os.environ:
            os.environ[key] = value


def _api_key(env_file: Path | None) -> str | None:
    if env_file is not None:
        _load_local_env(env_file)
    for name in ("RIGHT_API_KEY", "RIGHTAPI_API_KEY"):
        value = os.environ.get(name)
        if value:
            return value
    return None


def _data_url(path: Path) -> str:
    suffix = path.suffix.lower()
    mime = "image/jpeg" if suffix in (".jpg", ".jpeg") else "image/webp" if suffix == ".webp" else "image/png"
    return f"data:{mime};base64,{base64.b64encode(path.read_bytes()).decode('ascii')}"


def _request(url: str, method: str, key: str, payload: bytes | None = None) -> dict[str, Any]:
    headers = {"Authorization": f"Bearer {key}", "Accept": "application/json"}
    if payload is not None:
        headers["Content-Type"] = "application/json"
    request = urllib.request.Request(url, data=payload, headers=headers, method=method)
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            body = response.read()
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"Right API HTTP {error.code}: {detail[:500]}") from error
    except urllib.error.URLError as error:
        raise RuntimeError(f"Right API connection failed: {error.reason}") from error
    try:
        parsed = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise RuntimeError("Right API returned invalid JSON") from error
    if not isinstance(parsed, dict):
        raise RuntimeError("Right API response must be a JSON object")
    return parsed


def _submit(request_body: dict[str, Any], key: str) -> dict[str, Any]:
    return _request(SUBMIT_URL, "POST", key, json.dumps(request_body).encode("utf-8"))


def _poll(task_id: str, key: str, interval: float, timeout: float) -> dict[str, Any]:
    deadline = time.monotonic() + timeout
    while True:
        result = _request(TASK_URL.format(task_id=urllib.parse.quote(task_id, safe="")), "GET", key)
        status = str(result.get("status", "")).lower()
        if status in {"completed", "complete", "succeeded", "success"} or "data" in result or "candidates" in result:
            return result
        if status in {"failed", "error", "cancelled", "canceled"}:
            detail = result.get("error") or result.get("message") or status
            raise RuntimeError(f"Right image task failed: {detail}")
        if time.monotonic() >= deadline:
            raise TimeoutError(f"timed out waiting for task {task_id}")
        time.sleep(interval)


def _results(response: dict[str, Any]) -> list[tuple[str, bytes | None]]:
    values = response.get("data")
    if not isinstance(values, list):
        values = response.get("candidates")
    if not isinstance(values, list):
        raise RuntimeError("completed response has no data/candidates list")
    found: list[tuple[str, bytes | None]] = []
    for item in values:
        if not isinstance(item, dict):
            continue
        url = item.get("url")
        if isinstance(url, str) and url:
            found.append((url, None))
            continue
        encoded = item.get("b64_json") or item.get("base64")
        if isinstance(encoded, str) and encoded:
            try:
                found.append(("", base64.b64decode(encoded, validate=True)))
            except ValueError as error:
                raise RuntimeError("completed response contains invalid base64 image") from error
    if not found:
        raise RuntimeError("completed response contains no image URL or base64 payload")
    return found


def _download(url: str, key: str) -> bytes:
    request = urllib.request.Request(url, headers={"Authorization": f"Bearer {key}"})
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            return response.read()
    except (urllib.error.HTTPError, urllib.error.URLError) as error:
        raise RuntimeError(f"image download failed: {error}") from error


def _safe_manifest(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prompt", required=True, help="visual brief sent to the image model")
    parser.add_argument("--model", choices=("gpt-image-2", "gpt-image-2-vip"), default=DEFAULT_MODEL)
    parser.add_argument("--n", type=int, default=3, help="number of variants for one prompt")
    parser.add_argument("--size", default=DEFAULT_SIZE, help="Right aspect ratio or pixel size")
    parser.add_argument("--image-size", default=DEFAULT_IMAGE_SIZE, choices=("1K", "2K", "4K"))
    parser.add_argument("--reference", action="append", type=Path, default=[], help="reference image, repeatable")
    parser.add_argument("--output-dir", type=Path, default=Path("build/design-evidence/drafts"))
    parser.add_argument("--manifest", type=Path, help="manifest path; defaults to <output-dir>/job.json")
    parser.add_argument("--env-file", type=Path, default=Path(".env.local"))
    parser.add_argument("--poll-interval", type=float, default=4.0)
    parser.add_argument("--timeout", type=float, default=900.0)
    parser.add_argument("--confirm-paid", action="store_true", help="authorize the paid network request")
    parser.add_argument("--dry-run", action="store_true", help="print the request and do not call Right")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)
    if args.n < 1 or args.n > 8:
        raise SystemExit("--n must be between 1 and 8")
    if any(not path.is_file() for path in args.reference):
        missing = next(path for path in args.reference if not path.is_file())
        raise SystemExit(f"reference image does not exist: {missing}")
    body: dict[str, Any] = {
        "model": args.model,
        "prompt": args.prompt,
        "n": args.n,
        "size": args.size,
        "imageSize": args.image_size,
        "async": True,
    }
    if args.reference:
        body["image"] = [_data_url(path) for path in args.reference]
    summary = {key: value for key, value in body.items() if key != "image"}
    summary["reference_count"] = len(args.reference)
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    if args.dry_run or not args.confirm_paid:
        if not args.confirm_paid and not args.dry_run:
            print("dry-run: add --confirm-paid after reviewing the summary to submit", file=sys.stderr)
        return 0
    key = _api_key(args.env_file)
    if not key:
        raise SystemExit("missing RIGHT_API_KEY; set it in .env.local or the environment")
    output_dir = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = args.manifest or output_dir / "job.json"
    submitted = _submit(body, key)
    task_id = submitted.get("task_id")
    if not isinstance(task_id, str) or not task_id:
        raise RuntimeError("Right API submit response has no task_id")
    completed = _poll(task_id, key, args.poll_interval, args.timeout)
    files: list[dict[str, Any]] = []
    for index, (url, payload) in enumerate(_results(completed), start=1):
        data = payload if payload is not None else _download(url, key)
        filename = output_dir / f"draft-{index:02d}.png"
        filename.write_bytes(data)
        files.append({"path": str(filename.as_posix()), "sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data)})
    manifest = {
        "created_at": datetime.now(timezone.utc).isoformat(),
        "provider": "rightapi.ai",
        "model": args.model,
        "task_id": task_id,
        "request": summary,
        "files": files,
    }
    _safe_manifest(manifest_path, manifest)
    print(f"completed task {task_id}; wrote {len(files)} image(s) and {manifest_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, TimeoutError, OSError) as error:
        print(f"right image job error: {error}", file=sys.stderr)
        raise SystemExit(1)