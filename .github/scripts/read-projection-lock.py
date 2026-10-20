#!/usr/bin/env python3
"""Admit public, immutable dependency URLs only after the owner commits their lock."""
import argparse
import json
import os
from pathlib import Path
import re
from urllib.parse import urlsplit


def read_lock(path):
    if path.stat().st_size > 64 << 10:
        raise ValueError("Projection dependency lock exceeds 64 KiB")
    lock = json.loads(path.read_text())
    if lock.get("schema") != 1 or lock.get("qt_version") != "6.10.3" or lock.get("platform") != "linux-amd64":
        raise ValueError("Projection lock must bind the qualified Linux amd64 Qt ABI")
    dependencies = lock.get("dependencies", {})
    if set(dependencies) != {"SpatialPipe", "SpatialPipeScene", "ForgePipe"}:
        raise ValueError("Projection requires exactly the installed runtime, scene and engine SDKs")
    environment = {}
    for product, prefix in [("SpatialPipe", "SPATIALPIPE"), ("SpatialPipeScene", "SCENE"), ("ForgePipe", "FORGEPIPE")]:
        entry = dependencies[product]
        url = urlsplit(entry["url"])
        if (entry.get("version") != "0.1.0" or url.scheme != "https" or url.netloc != "github.com"
                or "/releases/download/" not in url.path or url.query or url.fragment
                or not re.fullmatch(r"[0-9a-f]{64}", entry.get("sha256", ""))):
            raise ValueError("Dependency must identify an exact public release archive and SHA-256")
        environment[prefix + "_URL"] = entry["url"]
        environment[prefix + "_SHA256"] = entry["sha256"]
    return environment


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    path = Path(__file__).resolve().parents[2] / "release/projection-dependencies.lock.json"
    if arguments.check:
        ready = path.is_file()
        if ready:
            read_lock(path)
        with Path(os.environ["GITHUB_OUTPUT"]).open("a") as output:
            output.write("ready=" + ("true" if ready else "false") + "\n")
        if not ready:
            print("::notice::Projection qualification skipped: exact public dependency lock is pending a separate approved PR.")
            with Path(os.environ["GITHUB_STEP_SUMMARY"]).open("a") as summary:
                summary.write("Projection qualification and publication are **skipped, pending the committed public dependency lock**. Browser SDK matrix remains independent.\n")
    else:
        environment = read_lock(path)
        with Path(os.environ["GITHUB_ENV"]).open("a") as output:
            for key, value in environment.items():
                output.write(key + "=" + value + "\n")
